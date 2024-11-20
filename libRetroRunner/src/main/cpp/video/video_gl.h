//
// Created by aidoo on 2024/11/5.
//

#ifndef _VIDEO_GL_H
#define _VIDEO_GL_H

#include <vector>
#include <memory>

#include <EGL/egl.h>
#include "../video_context.h"
#include "shader_pass.h"
#include "texture.h"

namespace libRetroRunner {


    class GLVideoContext : public VideoContext {

    public:
        GLVideoContext();

        ~GLVideoContext() override;

        void Init() override;

        void Reinit() override;

        void Destroy() override;

        void SetHolder(void *envObj, void *surfaceObj) override;

        void SetSurfaceSize(unsigned int width, unsigned int height) override;

        void OnGameGeometryChanged() override;

        void Prepare() override;

        void OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) override;

        void DrawFrame() override;

        unsigned int GetCurrentFramebuffer() override;

    private:

        void createPassChain();

    private:
        int screen_width;
        int screen_height;

        uint64_t frame_count;

        //游戏渲染目标
        std::unique_ptr<GLTextureObject> gameTexture;  //用于非硬件加速的模拟核心渲染
        std::vector<std::unique_ptr<GLShaderPass> > passes = std::vector<std::unique_ptr<GLShaderPass>>();  //总是会有1个pass,第0个用于游戏画面的渲染

        EGLDisplay eglDisplay;
        EGLSurface eglSurface;
        EGLContext eglContext;

        bool is_ready;


    };
}
#endif
