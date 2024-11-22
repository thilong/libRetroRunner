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

#include "video_context_gles.h"
#include "../../types/log.h"
#include "../../app/app_context.h"
#include "../../app/environment.h"
#include "../../app/setting.h"

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

    GLESVideoContext::GLESVideoContext() : VideoContext() {
        game_geometry_changed_ = true;
        is_ready = false;
        eglContext = nullptr;
        eglDisplay = nullptr;
        eglSurface = nullptr;
        is_hardware_accelerated = false;
        screen_height = 0;
        screen_height = 0;
        eglDisplay = EGL_NO_DISPLAY;
        eglSurface = EGL_NO_SURFACE;
        eglContext = EGL_NO_CONTEXT;
    }

    GLESVideoContext::~GLESVideoContext() {
        Destroy();
    }

    bool GLESVideoContext::Init() {
        if (eglDisplay == EGL_NO_DISPLAY || eglSurface == EGL_NO_SURFACE || eglContext == EGL_NO_CONTEXT) {
            LOGE("eglDisplay is not initialized.");
            return false;
        }
        if (eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) != EGL_TRUE) {
            LOGE("eglMakeCurrent failed.");
            return false;
        }

#if defined(HAVE_GLES3) && (ENABLE_GL_DEBUG)
        initializeGLESLogCallbackIfNeeded();
#endif

        auto appContext = AppContext::Current();
        auto env = appContext->GetEnvironment();
        core_pixel_format_ = env->GetCorePixelFormat();
        is_hardware_accelerated = env->GetRenderUseHWAcceleration();

        createPassChain();
        if (env->GetRenderUseHWAcceleration()) {
            env->InvokeRenderContextReset();
        }
        is_ready = true;
        LOGD_GLVIDEO("GLESVideoContext initialized, hardware accelerated: %d.", is_hardware_accelerated);
        return true;
    }

    void GLESVideoContext::Destroy() {
        is_ready = false;

        if (software_render_tex_) {
            software_render_tex_->Destroy();
            software_render_tex_ = nullptr;
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

    void GLESVideoContext::SetSurface(int argc, void **argv) {
        JNIEnv *env = (JNIEnv *) argv[0];
        jobject surface = (jobject) argv[1];

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

    void GLESVideoContext::OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) {
        if (!is_ready) {
            return;
        }
        /**
         * when core use software renderer, data won't be null if the video data filled.
         * when core use hardware renderer, data will be null for frame may not render complete.
         *      or RETRO_HW_FRAME_BUFFER_VALID for frame render complete.
         */
        if (data != nullptr) {
            if (data != RETRO_HW_FRAME_BUFFER_VALID) {
                auto appContext = AppContext::Current();
                /* we check the game texture size, create a texture buffer at  right size*/
                if (software_render_tex_ == nullptr || software_render_tex_->GetWidth() != width || software_render_tex_->GetHeight() != height) {
                    software_render_tex_ = std::make_unique<GLTextureObject>();
                    software_render_tex_->Create(width, height);
                }
                //render the data to our game texture, then use it as a texture for the first pass.
                software_render_tex_->WriteTextureData(data, width, height, core_pixel_format_);
                passes[0]->FillTexture(software_render_tex_->GetTexture());
            }
            DrawFrame();
        }
    }

    void GLESVideoContext::DrawFrame() {

        if (screen_width == 0 || screen_height == 0) {
            LOGW_GLVIDEO("draw frame failed: screen_width or screen_height is 0.");
            return;
        }
        do {
            /* we draw the passes in order, and fill the texture of the next pass with the texture of the previous pass.
             * this is prepared for shader processing.
             */
            std::vector<std::unique_ptr<GLShaderPass> >::iterator pass;
            GLShaderPass *prePass = nullptr;
            for (pass = passes.begin(); pass != passes.end(); pass++) {
                if (prePass != nullptr) {
                    (*pass)->FillTexture(prePass->GetTexture());
                }
                prePass = pass->get();
            }

            //check if we need to dump the frame to file
            if (!next_screenshot_store_path_.empty()) {
                passes.rbegin()->get()->DrawToFile(next_screenshot_store_path_);
                next_screenshot_store_path_.clear();
            }

            //draw the last pass to screen
            if (!passes.empty())
                passes.rbegin()->get()->DrawOnScreen(screen_width, screen_height);

            eglSwapBuffers(eglDisplay, eglSurface);

            //reset opengl es context for hardware acceleration
            if (is_hardware_accelerated) {
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

    void GLESVideoContext::SetSurfaceSize(unsigned int width, unsigned int height) {
        screen_width = width;
        screen_height = height;
    }

    unsigned int GLESVideoContext::GetCurrentFramebuffer() {
        if (passes.empty()) {
            return 0;
        }
        return passes[0]->GetFrameBuffer();
    }

    void GLESVideoContext::OnGameGeometryChanged() {

    }

    void GLESVideoContext::Prepare() {
        if (game_geometry_changed_) {
            if (passes.empty()) {
                createPassChain();
            } else {
                auto env = AppContext::Current()->GetEnvironment();
                auto setting = Setting::Current();
                std::unique_ptr<GLShaderPass> *pass = &passes[0];
                (*pass)->CreateFrameBuffer(env->GetGameWidth(), env->GetGameHeight(), setting->GetVideoUseLinear(), env->GetRenderUseDepth(), env->GetRenderUseDepth());
            }
            game_geometry_changed_ = false;
        }
    }

    void GLESVideoContext::createPassChain() {
        if (passes.empty()) {
            auto env = AppContext::Current()->GetEnvironment();
            auto setting = Setting::Current();

            std::unique_ptr<GLShaderPass> pass = std::make_unique<GLShaderPass>(nullptr, nullptr);
            pass->SetPixelFormat(core_pixel_format_);
            pass->SetHardwareAccelerated(is_hardware_accelerated);
            pass->CreateFrameBuffer(env->GetGameWidth(), env->GetGameHeight(), setting->GetVideoUseLinear(), env->GetRenderUseDepth(), env->GetRenderUseDepth());
            passes.push_back(std::move(pass));
        }
    }

    bool GLESVideoContext::TakeScreenshot(const std::string &path) {
        if (passes.empty()) {
            return false;
        }
        passes.rbegin()->get()->DrawToFile(path);
        return true;
    }

}
