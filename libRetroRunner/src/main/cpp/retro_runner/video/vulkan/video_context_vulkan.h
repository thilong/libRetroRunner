//
// Created by aidoo on 1/24/2025.
//
#include <vector>
#include <memory>

#include <EGL/egl.h>
#include "../video_context.h"

#ifndef LIBRETRORUNNER_VIDEO_CONTEXT_VULKAN_H
#define LIBRETRORUNNER_VIDEO_CONTEXT_VULKAN_H


namespace libRetroRunner {

    class VulkanVideoContext : public VideoContext {
    public:
        VulkanVideoContext();

        ~VulkanVideoContext() override;

        bool Init() override;

        void Destroy() override;

        void Unload() override;

        void SetSurface(int argc, void **argv) override;

        void SetSurfaceSize(unsigned int width, unsigned int height) override;

        void Prepare() override;

        void OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) override;

        void DrawFrame() override;

        unsigned int GetCurrentFramebuffer() override;

        bool TakeScreenshot(const std::string &path) override;

    private:
        class VulkanContext* vulkan_context_ = nullptr;
    };
}


#endif //LIBRETRORUNNER_VIDEO_CONTEXT_VULKAN_H
