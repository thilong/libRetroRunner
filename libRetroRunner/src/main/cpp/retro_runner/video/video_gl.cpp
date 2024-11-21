//
// Created by aidoo on 2024/11/5.
//

#include <unistd.h>

#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "video_gl.h"
#include "../rr_log.h"
#include "../app.h"
#include "../environment.h"


#define LOGD_GLVIDEO(...) LOGD("[VIDEO] " __VA_ARGS__)
#define LOGW_GLVIDEO(...) LOGW("[VIDEO] " __VA_ARGS__)
#define LOGE_GLVIDEO(...) LOGE("[VIDEO] " __VA_ARGS__)
#define LOGI_GLVIDEO(...) LOGI("[VIDEO] " __VA_ARGS__)

#define ENABLE_GL_DEBUG 1

namespace libRetroRunner {

    extern "C" JavaVM *gVm;

#ifdef HAVE_GLES3

    static void MessageCallback(
            GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar *message,
            const void *userParam
    ) {
        if (type == GL_DEBUG_TYPE_ERROR) {
            LOGE("GL CALLBACK: \"** GL ERROR **\" type = 0x%x, severity = 0x%x, message = %s\n",
                 type,
                 severity,
                 message);
        }
    }

    __unused
    static bool initializeGLESLogCallbackIfNeeded() {
        auto debugCallback = (void (*)(void *, void *)) eglGetProcAddress("glDebugMessageCallback");
        if (debugCallback) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            debugCallback((void *) MessageCallback, nullptr);
            LOGW("GL debug callback enabled.");
        } else {
            LOGW("cant find symbol of glDebugMessageCallback, log wont show up.");
        }
        return true;
    }

#endif

}

namespace libRetroRunner {

    GLVideoContext::GLVideoContext() : VideoContext() {

        is_ready = false;
        eglContext = nullptr;
        eglDisplay = nullptr;
        eglSurface = nullptr;
        frame_count = 0;
        screen_height = 0;
        screen_height = 0;
    }

    GLVideoContext::~GLVideoContext() {
        Destroy();
    }

    void GLVideoContext::Init() {
        if (eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) != EGL_TRUE) {
            LOGE("eglMakeCurrent failed.");
            return;
        }

#if defined(HAVE_GLES3) && (ENABLE_GL_DEBUG)
        initializeGLESLogCallbackIfNeeded();
#endif
        createPassChain();
        auto appContext = AppContext::Current();
        auto env = appContext->GetEnvironment();
        if (env->renderUseHWAcceleration) {
            env->renderContextReset();  //这里，DC游戏调用后导致无法显示。APP暂时没有调用core->run, 正在调试中
        }
        is_ready = true;
    }

    void GLVideoContext::Destroy() {
        if (gameTexture) {
            gameTexture->Destroy();
            gameTexture = nullptr;
        }
        //循环调用passes的Destroy
        for (auto &pass: passes) {
            pass->Destroy();
        }
        passes.erase(passes.begin(), passes.end());

        if (eglDisplay != EGL_NO_DISPLAY) {

            if (eglSurface != EGL_NO_SURFACE) {
                //LOGW("eglDestroySurface.");
                //eglDestroySurface(eglDisplay, eglSurface);
                eglSurface = EGL_NO_SURFACE;
            }
            if (eglContext != EGL_NO_CONTEXT) {
                eglDestroyContext(eglDisplay, eglContext);
                eglContext = EGL_NO_CONTEXT;
            }
            eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglTerminate(eglDisplay);
            eglDisplay = EGL_NO_DISPLAY;
        }
    }

    void GLVideoContext::SetHolder(void *envObj, void *surfaceObj) {
        JNIEnv *env = (JNIEnv *) envObj;
        jobject surface = (jobject) surfaceObj;

        ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
        EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            LOGE_GLVIDEO("egl have not got display.");
            return;
        }
        if (eglInitialize(display, 0, 0) != EGL_TRUE) {
            LOGE_GLVIDEO("egl Initialize failed.%d", eglGetError());
            return;
        }
        eglDisplay = display;
        //2:EGL_OPENGL_ES2_BIT   3:EGL_OPENGL_ES3_BIT_KHR
        const EGLint atrrs[] = {
                EGL_ALPHA_SIZE, 8,
                EGL_RED_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_DEPTH_SIZE, 16,
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_NONE
        };
        //opengl es2: EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGLConfig eglConfig;
        EGLint numOfEglConfig;
        if (eglChooseConfig(display, atrrs, &eglConfig, 1, &numOfEglConfig) != EGL_TRUE) {
            LOGE_GLVIDEO("egl choose config failed.%d,", eglGetError());
            return;
        }

        EGLint attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        eglContext = eglCreateContext(display, eglConfig, nullptr, attributes);
        if (!eglContext) {
            LOGE_GLVIDEO("eglCreateContext failed.");
            return;
        }

        EGLint format;
        if (!eglGetConfigAttrib(display, eglConfig, EGL_NATIVE_VISUAL_ID, &format)) {
            LOGE_GLVIDEO("egl get config attrib failed.");
            return;
        }

        ANativeWindow_acquire(window);
        ANativeWindow_setBuffersGeometry(window, 0, 0, format);
        EGLint window_attribs[] = {
                EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
                EGL_NONE,
        };
        eglSurface = eglCreateWindowSurface(display, eglConfig, window, window_attribs);
        if (!eglSurface) {
            LOGE_GLVIDEO("eglCreateWindowSurface failed.");
            return;
        }


    }

    void GLVideoContext::OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) {
        if (is_ready && data != nullptr) {
            if (data != RETRO_HW_FRAME_BUFFER_VALID) {
                auto appContext = AppContext::Current();
                //如果没有使用硬件加速，则需要创建一个软件纹理
                if (gameTexture == nullptr || gameTexture->GetWidth() != width || gameTexture->GetHeight() != height) {
                    gameTexture = std::make_unique<GLTextureObject>();
                    gameTexture->Create(width, height);
                    GL_CHECK2("gameTexture->Create", "frame: %llu", frame_count);
                }
                //把核心渲染的数据写入到gameTexture上
                gameTexture->WriteTextureData(data, width, height, appContext->GetEnvironment()->pixelFormat);

                passes[0]->FillTexture(gameTexture->GetTexture());
            }
            DrawFrame();
        }
        frame_count++;
    }

    void GLVideoContext::DrawFrame() {

        if (screen_width == 0 || screen_height == 0) {
            LOGW_GLVIDEO("draw frame failed: screen_width or screen_height is 0.");
            return;
        }
        do {
            //循环调用passes的DrawToScreen
            std::vector<std::unique_ptr<GLShaderPass> >::iterator pass;
            GLShaderPass *prePass = nullptr;

            for (pass = passes.begin(); pass != passes.end(); pass++) {
                if (prePass != nullptr) {
                    (*pass)->FillTexture(prePass->GetTexture());
                }
                prePass = pass->get();
            }
            if (!dumpPath.empty()) {
                passes.rbegin()->get()->DrawToFile(dumpPath);
                dumpPath.clear();
            }

            if (!passes.empty())
                passes.rbegin()->get()->DrawOnScreen(screen_width, screen_height);

            eglSwapBuffers(eglDisplay, eglSurface);

            if (true) {  //只在硬件渲染下使用，用于每一帧的清理
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_CULL_FACE);
                glDisable(GL_DITHER);

                glDisable(GL_STENCIL_TEST);
                glDisable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glBlendEquation(GL_FUNC_ADD);
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            }

        } while ((false));

    }

    void GLVideoContext::SetSurfaceSize(unsigned int width, unsigned int height) {
        screen_width = width;
        screen_height = height;
    }

    unsigned int GLVideoContext::GetCurrentFramebuffer() {
        if (passes.empty()) {
            return 0;
        }
        return passes[0]->GetFrameBuffer();
    }

    void GLVideoContext::OnGameGeometryChanged() {

    }

    void GLVideoContext::Prepare() {
        auto env = AppContext::Current()->GetEnvironment();
        if (env->gameGeometryChanged) {
            createPassChain();
            std::unique_ptr<GLShaderPass> *pass = &passes[0];
            (*pass)->CreateFrameBuffer(env->gameGeometryWidth, env->gameGeometryHeight, env->renderUseLinear, env->renderUseDepth, env->renderUseStencil);
            env->gameGeometryChanged = false;
        }
    }

    void GLVideoContext::createPassChain() {
        auto env = AppContext::Current()->GetEnvironment();
        if (passes.empty()) {
            std::unique_ptr<GLShaderPass> pass = std::make_unique<GLShaderPass>(nullptr, nullptr);
            pass->SetPixelFormat(env->pixelFormat);
            pass->SetHardwareAccelerated(env->renderUseHWAcceleration);
            pass->CreateFrameBuffer(env->gameGeometryWidth, env->gameGeometryHeight, env->renderUseLinear, env->renderUseDepth, env->renderUseStencil);
            passes.push_back(std::move(pass));
        }
    }

    void GLVideoContext::Reinit() {

    }
}
