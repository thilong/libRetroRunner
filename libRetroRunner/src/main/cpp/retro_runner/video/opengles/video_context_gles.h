//
// Created by Aidoo.TK on 2024/11/5.
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

        void Unload() override;


        bool SurfaceChanged(void *surface, unsigned width, unsigned height) override;

        void SetSurface(int argc, void **argv) override;

        void SetSurfaceSize(unsigned int width, unsigned int height) override;

        void Prepare() override;

        void OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) override;

        void DrawFrame() override;

        unsigned int GetCurrentFramebuffer() override;

        bool TakeScreenshot(const std::string &path) override;

        void setHWRenderContextNegotiationInterface(const void *) override {}

    private:

        void createPassChain();

    private:
        int screen_width_;
        int screen_height_;

        /* pixel format of core use */
        int core_pixel_format_;

        bool is_hardware_accelerated_;
        /**
         * game output video ration
         * 0: 0, 1: -90, 2: -180, 3: -270 (degrees)
         */
        unsigned int game_video_ration_;

        /** for software game render, fill with game video data on every frame.*/
        std::unique_ptr<GLTextureObject> software_render_tex_;

        /**
         * for shader and game texture render.
         * game frame data is rendered into the [0] GLShaderPass, and then passes render
         * the framebuffer to the next, eg: [0] -> [1] -> [2] -> ...
         * at last, the end one of the passes will render the texture to screen.
         */
        std::vector<std::unique_ptr<GLShaderPass> > passes_ = std::vector<std::unique_ptr<GLShaderPass>>();

        EGLDisplay egl_display_;
        EGLSurface egl_surface_;
        EGLContext egl_context_;
        EGLConfig egl_config_;
        bool egl_initialized_;


        bool is_ready_;


    };
}
#endif
