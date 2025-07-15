//
// Created by aidoo on 3/3/2025.
//

#ifndef LIBRETRORUNNER_RR_VULKAN_H
#define LIBRETRORUNNER_RR_VULKAN_H

#include "vulkan_wrapper.h"
#include <vector>
#include <string>

#include "../../types/log.h"

#define LOGD_VC(...) LOGD("[Vulkan] " __VA_ARGS__)
#define LOGW_VC(...) LOGW("[Vulkan] " __VA_ARGS__)
#define LOGE_VC(...) LOGE("[Vulkan] " __VA_ARGS__)
#define LOGI_VC(...) LOGI("[Vulkan] " __VA_ARGS__)

enum VulkanCustomPixelFormat {
    PIXEL_FORMAT_0RGB1555 = 0,
    PIXEL_FORMAT_XRGB8888 = 1,
    PIXEL_FORMAT_RGB565 = 2,
    PIXEL_FORMAT_UNKNOWN = INT_MAX
};

enum VulkanShaderType {
    SHADER_VERTEX, SHADER_FRAGMENT
};

namespace VkUtil {
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);


    VkCommandBuffer beginSingleTimeCommands(VkDevice logicalDevice, VkCommandPool commandPool);

    void endSingleTimeCommands(VkDevice logicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer);

    void transitionImageLayout(VkDevice logicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image, VkFormat format, VkImageLayout newLayout, VkImageLayout *imgLayout);

    void updateDescriptorSet(VkDevice logicalDevice, VkDescriptorSet descriptorSet, VkImageView imageView, VkSampler sampler);

    bool MapMemoryTypeToIndex(VkPhysicalDevice physicalDevice, uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);

    void convertRGB565ToRGBA8888(const void *data, unsigned int width, unsigned int height, unsigned char *buffer);
}

#endif
