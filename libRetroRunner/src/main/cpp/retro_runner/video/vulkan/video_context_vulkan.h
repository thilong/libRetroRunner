//
// Created by aidoo on 1/24/2025.
//
#include <vector>
#include <memory>

#include "../video_context.h"

#include <libretro-common/include/libretro_vulkan.h>

#ifndef LIBRETRORUNNER_VIDEO_CONTEXT_VULKAN_H
#define LIBRETRORUNNER_VIDEO_CONTEXT_VULKAN_H

class VulkanSamplingTexture;

class VulkanRWBuffer;



namespace libRetroRunner {

    class VulkanInstance;

    class VulkanPipeline;

    class VulkanSwapchain;

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

        void setHWRenderContextNegotiationInterface(const void *interface) override;

    private:
        void recordCommandBufferForSoftwareRender(void *pCommandBuffer, uint32_t imageIndex);

        void vulkanCommitFrame();

    private:
        void *window_{};
        /* pixel format of core use */
        int core_pixel_format_;


        VulkanInstance *vulkanInstance_{};
        VulkanPipeline *vulkanPipeline_{};
        VulkanSwapchain *vulkanSwapchain_{};

        VulkanSamplingTexture *softwareTexture_{};
        VulkanRWBuffer *vertexBuffer_{};

        bool vulkanIsReady_{};
        uint32_t width_;
        uint32_t height_;

        const retro_hw_render_context_negotiation_interface_vulkan *retroHWNegotiationInterface_{};


    };
}


#endif //LIBRETRORUNNER_VIDEO_CONTEXT_VULKAN_H
