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
        bool valid = false;

        VkFormat format = VK_FORMAT_UNDEFINED;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineCache pipelineCache = VK_NULL_HANDLE;

        char *vertexShaderCode = nullptr;
        char *fragmentShaderCode = nullptr;
        VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
        VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
    };

    struct RRVulkanSwapchainContext {
        bool valid = false;
        long surfaceId = 0;

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkExtent2D extent{};
        VkFormat format = VK_FORMAT_UNDEFINED;
        uint32_t minImageCount = 0;

        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;

        uint32_t imageCount = 0;
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

        VkDevice logicalDevice_;
        VkQueue graphicQueue_;

        VkCommandPool commandPool_;


        RRVulkanRenderContext renderContext_;
        RRVulkanSwapchainContext swapchainContext_;

        retro_vulkan_destroy_device_t destroyDeviceImpl_;

    };
}


#endif //LIBRETRORUNNER_VIDEO_CONTEXT_VULKAN_H
