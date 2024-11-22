//
// Created by aidoo on 2024/11/5.
//

#ifndef _VIDEO_CONTEXT_GLES_H
#define _VIDEO_CONTEXT_GLES_H

#include <vector>
#include <memory>

#include <EGL/egl.h>
#include "../video_context.h"
#include "shader_pass.h"
#include "texture.h"

namespace libRetroRunner {


    class GLESVideoContext : public VideoContext {

    public:
        GLESVideoContext();

        ~GLESVideoContext() override;

        bool Init() override;

        void Destroy() override;

        void SetSurface(int argc, void **argv) override;

        void SetSurfaceSize(unsigned int width, unsigned int height) override;

        void OnGameGeometryChanged() override;

        void Prepare() override;

        void OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) override;

        void DrawFrame() override;

        unsigned int GetCurrentFramebuffer() override;

        void SetCoreOutputPixelFormat(int format) override {
            core_pixel_format_ = format;
        }

        bool TakeScreenshot(const std::string &path) override;

    private:

        void createPassChain();

    private:
        int screen_width;
        int screen_height;
        /* pixel format of core use */
        int core_pixel_format_;
        bool is_hardware_accelerated;
        //游戏渲染目标
        std::unique_ptr<GLTextureObject> software_render_tex_;  //用于非硬件加速的模拟核心渲染
        std::vector<std::unique_ptr<GLShaderPass> > passes = std::vector<std::unique_ptr<GLShaderPass>>();  //总是会有1个pass,第0个用于游戏画面的渲染

        EGLDisplay eglDisplay;
        EGLSurface eglSurface;
        EGLContext eglContext;

        bool is_ready;


    };
}
#endif
