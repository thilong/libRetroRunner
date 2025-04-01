//
// Created by Aidoo.TK on 2024/11/5.
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
#include "../../types/retro_types.h"

#define LOGD_GLVIDEO(...) LOGD("[VIDEO] " __VA_ARGS__)
#define LOGW_GLVIDEO(...) LOGW("[VIDEO] " __VA_ARGS__)
#define LOGE_GLVIDEO(...) LOGE("[VIDEO] " __VA_ARGS__)
#define LOGI_GLVIDEO(...) LOGI("[VIDEO] " __VA_ARGS__)

#define ENABLE_GL_DEBUG 1

namespace libRetroRunner {
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
        getHWProcAddress = (rr_hardware_render_proc_address_t) &eglGetProcAddress;
        is_ready_ = false;
        egl_initialized_ = false;
        egl_context_ = nullptr;
        egl_display_ = nullptr;
        egl_surface_ = nullptr;
        egl_config_ = nullptr;
        egl_format_ = 0;
        is_hardware_accelerated_ = false;
        screen_width_ = 0;
        screen_height_ = 0;
        egl_display_ = EGL_NO_DISPLAY;
        egl_surface_ = EGL_NO_SURFACE;
        egl_context_ = EGL_NO_CONTEXT;
    }

    GLESVideoContext::~GLESVideoContext() {
        Destroy();
    }

    void GLESVideoContext::Destroy() {
        is_ready_ = false;
        enabled_ = false;
        if (software_render_tex_) {
            software_render_tex_->Destroy();
            software_render_tex_ = nullptr;
        }
        //循环调用passes的Destroy
        for (auto &pass: passes_) {
            pass->Destroy();
        }
        passes_.erase(passes_.begin(), passes_.end());

        if (egl_display_ != EGL_NO_DISPLAY) {

            if (egl_surface_ != EGL_NO_SURFACE) {
                //LOGW("eglDestroySurface.");
                //eglDestroySurface(egl_display_, egl_surface_);
                egl_surface_ = EGL_NO_SURFACE;
            }
            if (egl_context_ != EGL_NO_CONTEXT) {
                eglDestroyContext(egl_display_, egl_context_);
                egl_context_ = EGL_NO_CONTEXT;
            }
            eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglTerminate(egl_display_);
            egl_display_ = EGL_NO_DISPLAY;
        }
    }

    bool GLESVideoContext::Load() {
        if (!egl_initialized_) {
            egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            if (egl_display_ == EGL_NO_DISPLAY) {
                LOGE_GLVIDEO("egl have not got display.");
                return false;
            }
            if (eglInitialize(egl_display_, 0, 0) != EGL_TRUE) {
                LOGE_GLVIDEO("egl Initialize failed.%d", eglGetError());
                return false;
            }
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
            EGLint numOfEglConfig;
            if (eglChooseConfig(egl_display_, atrrs, &egl_config_, 1, &numOfEglConfig) != EGL_TRUE) {
                LOGE_GLVIDEO("egl choose config failed.%d,", eglGetError());
                return false;
            }

            EGLint attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
            egl_context_ = eglCreateContext(egl_display_, egl_config_, nullptr, attributes);
            if (!egl_context_) {
                LOGE_GLVIDEO("eglCreateContext failed.");
                return false;
            }
            if (!eglGetConfigAttrib(egl_display_, egl_config_, EGL_NATIVE_VISUAL_ID, &egl_format_)) {
                LOGE_GLVIDEO("egl get config attrib failed.");
                return false;
            }
            LOGI_GLVIDEO("egl initialized.");
            egl_initialized_ = true;
        }

        auto app = AppContext::Current();
        AppWindow appWindow = app->GetAppWindow();
        if (surface_id_ == appWindow.surfaceId) return true;
        ANativeWindow_setBuffersGeometry(appWindow.window, 0, 0, egl_format_);
        EGLint window_attribs[] = {
                EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
                EGL_NONE,
        };
        egl_surface_ = eglCreateWindowSurface(egl_display_, egl_config_, appWindow.window, window_attribs);
        if (!egl_surface_) {
            LOGE_GLVIDEO("eglCreateWindowSurface failed, 0x%x", eglGetError());
            return false;
        }

        if (egl_display_ == EGL_NO_DISPLAY || egl_context_ == EGL_NO_CONTEXT) {
            LOGE("egl_display_ is not allReady.");
            return false;
        }
        if (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_) != EGL_TRUE) {
            LOGE("eglMakeCurrent failed.");
            return false;
        }
        LOGD_GLVIDEO("eglMakeCurrent %p,  thread: %d", egl_surface_, gettid());
        GL_CHECK("GLESVideoContext::Init 0");
#if defined(HAVE_GLES3) && (ENABLE_GL_DEBUG)
        initializeGLESLogCallbackIfNeeded();
#endif
        screen_width_ = appWindow.width;
        screen_height_ = appWindow.height;

        auto coreCtx = app->GetCoreRuntimeContext();
        auto gameCtx = app->GetGameRuntimeContext();
        core_pixel_format_ = coreCtx->GetPixelFormat();
        is_hardware_accelerated_ = coreCtx->GetRenderUseHardwareAcceleration();

        createPassChain();
        if (is_hardware_accelerated_) {
            retro_hw_context_reset_t reset_func = coreCtx->GetRenderHWContextResetCallback();
            if (reset_func) reset_func();
        }
        is_ready_ = true;
        enabled_ = true;
        surface_id_ = (long) appWindow.surfaceId;
        LOGD_GLVIDEO("GLESVideoContext allReady, hardware accelerated: %d.", is_hardware_accelerated_);
        return true;
    }

    void GLESVideoContext::Unload() {
        is_ready_ = false;
        enabled_ = false;
        surface_id_ = 0;

        auto appContext = AppContext::Current();
        auto coreCtx = appContext->GetCoreRuntimeContext();
        auto gameCtx = appContext->GetGameRuntimeContext();
        if (is_hardware_accelerated_) {
            retro_hw_context_reset_t destroy_func = coreCtx->GetRenderHWContextDestroyCallback();
            if (destroy_func) destroy_func();
        }
        egl_surface_ = EGL_NO_SURFACE;
        eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    }

    void GLESVideoContext::UpdateVideoSize(unsigned width, unsigned height) {
        screen_width_ = width;
        screen_height_ = height;
        LOGD_GLVIDEO("screen size changed: %d x %d", width, height);
    }

    void GLESVideoContext::OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) {
        if (!is_ready_ || !enabled_) {
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
                // create a texture buffer at  right size
                if (software_render_tex_ == nullptr || software_render_tex_->GetWidth() != width || software_render_tex_->GetHeight() != height) {
                    software_render_tex_ = std::make_unique<GLTextureObject>();
                    software_render_tex_->Create(width, height);
                }
                //render the data to our game texture, then use it as a texture for the first pass.
                software_render_tex_->WriteTextureData(data, width, height, core_pixel_format_);
                passes_[0]->FillTexture(software_render_tex_->GetTexture());
            }
            DrawFrame();
        }
    }

    void GLESVideoContext::DrawFrame() {
        if (!enabled_)return;
        if (screen_width_ == 0 || screen_height_ == 0) {
            LOGW_GLVIDEO("draw frame failed: screen_width_ or screen_height_ is 0.");
            return;
        }
        do {
            /* we draw the passes_ in order, and fill the texture of the next pass with the texture of the previous pass.
             * this is prepared for shader processing.
             */
            std::vector<std::unique_ptr<GLShaderPass> >::iterator pass;
            GLShaderPass *prePass = nullptr;
            for (pass = passes_.begin(); pass != passes_.end(); pass++) {
                if (prePass != nullptr) {
                    (*pass)->FillTexture(prePass->GetTexture());
                }
                prePass = pass->get();
            }

            //check if we need to dump the frame to file
            if (!next_screenshot_store_path_.empty()) {
                passes_.rbegin()->get()->DrawToFile(next_screenshot_store_path_);
                next_screenshot_store_path_.clear();
            }

            //draw the last pass to screen
            if (!passes_.empty())
                passes_.rbegin()->get()->DrawOnScreen(screen_width_, screen_height_);
            glFinish();
            eglSwapBuffers(egl_display_, egl_surface_);

            //reset opengl es context for hardware acceleration
            if (is_hardware_accelerated_) {
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

    unsigned int GLESVideoContext::GetCurrentFramebuffer() {
        if (passes_.empty()) {
            return 0;
        }
        return passes_[0]->GetFrameBuffer();
    }

    void GLESVideoContext::Prepare() {
        auto gameCtx = game_runtime_ctx_.lock();
        if (gameCtx) {
            game_video_ration_ = gameCtx->GetGeometryRotation() % 4;
            if (gameCtx->GetIsGeometryChanged()) {
                if (passes_.empty()) {
                    createPassChain();
                } else {
                    auto coreCtx = AppContext::Current()->GetCoreRuntimeContext();
                    auto setting = Setting::Current();
                    std::unique_ptr<GLShaderPass> *pass = &passes_[0];
                    (*pass)->CreateFrameBuffer(gameCtx->GetGeometryWidth(), gameCtx->GetGeometryHeight(), setting->GetVideoUseLinear(), coreCtx->GetRenderUseDepth(), coreCtx->GetRenderUseStencil());
                }
                gameCtx->SetGeometryChanged(false);
            }
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GLESVideoContext::createPassChain() {
        if (passes_.empty()) {
            auto gameCtx = AppContext::Current()->GetGameRuntimeContext();
            auto coreCtx = AppContext::Current()->GetCoreRuntimeContext();
            auto setting = Setting::Current();

            std::unique_ptr<GLShaderPass> pass = std::make_unique<GLShaderPass>(nullptr, nullptr);
            pass->SetPixelFormat(core_pixel_format_);
            pass->SetHardwareAccelerated(is_hardware_accelerated_);
            pass->CreateFrameBuffer(gameCtx->GetGeometryWidth(), gameCtx->GetGeometryHeight(), setting->GetVideoUseLinear(), coreCtx->GetRenderUseDepth(), coreCtx->GetRenderUseStencil());
            passes_.push_back(std::move(pass));
            LOGD_GLVIDEO("create pass chain, pass size: %zu", passes_.size());
        }
    }

    bool GLESVideoContext::TakeScreenshot(const std::string &path) {
        if (passes_.empty() || !is_ready_ || !enabled_) {
            LOGE_GLVIDEO("Can't write data to screenshot file since the video context is not valid.");
            return false;
        }
        passes_.rbegin()->get()->DrawToFile(path);
        return true;
    }


}
