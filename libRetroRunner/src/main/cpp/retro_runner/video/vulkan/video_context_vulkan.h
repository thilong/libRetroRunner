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
    enum VulkanShaderType {
        SHADER_VERTEX, SHADER_FRAGMENT
    };

    struct RRVulkanRenderContext {
        bool valid;

        VkFormat format;
        VkRenderPass renderPass;
        VkDescriptorSet descriptorSet;
        VkDescriptorPool descriptorPool;

        VkDescriptorSetLayout descriptorSetLayout;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        VkPipelineCache pipelineCache;

        char *vertexShaderCode;
        char *fragmentShaderCode;
        VkShaderModule vertexShaderModule;
        VkShaderModule fragmentShaderModule;
    };

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

        bool getRetroHardwareRenderInterface(void **) override;


    public:
        VkInstance retro_vulkan_create_instance_wrapper(const VkInstanceCreateInfo *create_info);

        VkDevice retro_vulkan_create_device_wrapper(VkPhysicalDevice gpu, const VkDeviceCreateInfo *create_info);

    private:
        void recordCommandBufferForSoftwareRender(void *pCommandBuffer, uint32_t imageIndex);

        void vulkanCommitFrame();

        bool vulkanCreateInstanceIfNeeded();

        bool vulkanCreateSurfaceIfNeeded();

        bool vulkanChooseGPU();

        bool vulkanCreateDeviceIfNeeded();

        bool vulkanCreateCommandPoolIfNeeded();

        bool vulkanCreateSwapchainIfNeeded();

        bool vulkanClearSwapchainIfNeeded();

        bool vulkanCreateRenderContextIfNeeded();

        void clearVulkanRenderContext();
        bool createShader(void *source, size_t sourceLength, VulkanShaderType shaderType, VkShaderModule *shader);
    private:
        const retro_hw_render_context_negotiation_interface_vulkan *getNegotiationInterface();

    private:
        void *window_;
        long surface_id_ = 0;
        int core_pixel_format_;

        uint32_t screen_width_;
        uint32_t screen_height_;
        bool is_ready_;

        retro_vulkan_context *retro_vk_context_;
        retro_hw_render_interface_vulkan *retro_render_interface_;
        VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures{};

        bool vulkanIsReady_{};


        VkInstance instance_;
        VkPhysicalDevice physicalDevice_;
        VkPhysicalDeviceProperties physicalDeviceProperties_{};
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties_{};

        uint32_t queueFamilyIndex_;

        VkSurfaceKHR surface_;

        VkDevice logicalDevice_;
        VkQueue graphicQueue_;

        VkCommandPool commandPool_;


        RRVulkanRenderContext renderContext_;


        retro_vulkan_destroy_device_t destroyDeviceImpl_;

    };
}


#endif //LIBRETRORUNNER_VIDEO_CONTEXT_VULKAN_H
