//
// Created by aidoo on 3/3/2025.
//

#ifndef LIBRETRORUNNER_VULKAN_CONTEXT_H
#define LIBRETRORUNNER_VULKAN_CONTEXT_H

#include "vulkan_api_wrapper.h"
#include "rr_vulkan_types.h"

namespace libRetroRunner {
    class VulkanContext {
    public:
        VulkanContext();

        ~VulkanContext();

    public:

        void Destroy();

    private:
        static void debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                  VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                  void *pUserData);
        void logVkExtensions();
        bool createInstance();
        bool findPhysicalDevice();
        bool createLogicalDevice();

        bool attachWindow(void *window);
        bool createSwapchain();
        bool createImageView();

        bool createRenderPass();
        bool createVkShaderModule(const uint32_t *content, size_t size, VulkanShaderType type, VkShaderModule *shaderOut) const;
        bool createVkGraphicsPipeline();
        bool createVkCommandPool();

        bool createVkCommandBuffers();
        bool createVkFrameBuffers();
        bool createVkSyncObjects();

        void deleteSwapChain();
        void deleteGraphicsPipeline() ;
        void deleteBuffers() ;

        static void setVkImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
                                     VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                                     VkPipelineStageFlags srcStages,
                                     VkPipelineStageFlags destStages);

    private:


        VulkanDeviceContext device_info_{};
        VulkanSwapchainContext swapchain_info_{};
        VulkanPipelineContext pipeline_info_{};
        VulkanRenderContext render_info_{};
    };


}

#endif //LIBRETRORUNNER_VULKAN_CONTEXT_H
