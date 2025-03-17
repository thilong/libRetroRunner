//
// Created by aidoo on 3/3/2025.
//

#ifndef LIBRETRORUNNER_RR_VULKAN_TYPES_H
#define LIBRETRORUNNER_RR_VULKAN_TYPES_H

#include "vulkan_api_wrapper.h"
#include <vector>

struct VulkanDeviceContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice logical_device = VK_NULL_HANDLE;
    uint32_t queue_family_index = 0;
    VkQueue queue = VK_NULL_HANDLE;
};

struct VulkanSwapchainContext {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat display_format = VK_FORMAT_UNDEFINED;
    VkExtent2D display_extent = {0, 0};
    VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    uint32_t image_count = 0;

    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
    std::vector<VkFramebuffer> frame_buffers;
};

struct VulkanPipelineContext {
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipelineCache cache = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

struct VulkanRenderContext {
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer *command_buffer = nullptr;
    uint32_t command_buffer_len = 0;
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
};

enum VulkanShaderType {
    SHADER_VERTEX, SHADER_FRAGMENT
};

#endif //LIBRETRORUNNER_RR_VULKAN_TYPES_H
