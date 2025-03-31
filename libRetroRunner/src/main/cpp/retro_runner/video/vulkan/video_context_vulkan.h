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

        bool Load() override;

        void Unload() override;

        void UpdateVideoSize(unsigned width, unsigned height) override;

        void Destroy() override;

        void Prepare() override;

        void OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) override;

        void DrawFrame() override;

        bool TakeScreenshot(const std::string &path) override;

        void setHWRenderContextNegotiationInterface(const void *interface) override;

    private:
        void recordCommandBufferForSoftwareRender(void *pCommandBuffer, uint32_t imageIndex);

        void vulkanCommitFrame();

    private:
        void *window_{};
        int core_pixel_format_;

        uint32_t screen_width_;
        uint32_t screen_height_;
        bool is_ready_;

        retro_vulkan_context *vk_context_{};

        VulkanInstance *vulkanInstance_{};
        VulkanPipeline *vulkanPipeline_{};
        VulkanSwapchain *vulkanSwapchain_{};
        VulkanSamplingTexture *softwareTexture_{};
        VulkanRWBuffer *vertexBuffer_{};

        bool vulkanIsReady_{};


        const retro_hw_render_context_negotiation_interface_vulkan *retroHWNegotiationInterface_{};

    };
}


#endif //LIBRETRORUNNER_VIDEO_CONTEXT_VULKAN_H
