//
// Created by aidoo.
//

#ifndef LIBRETRORUNNER_VK_READ_WRITE_BUFFER_H
#define LIBRETRORUNNER_VK_READ_WRITE_BUFFER_H

#include "rr_vulkan.h"

class VulkanRWBuffer {
public:
    VulkanRWBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, uint32_t queueFamilyIndex);

    ~VulkanRWBuffer();

    bool create(VkDeviceSize size, VkBufferUsageFlags usage);

    void update(const void *data, VkDeviceSize size);

    void destroy();

public:
    inline VkBuffer getBuffer() const { return buffer_; }

    inline VkDeviceMemory getMemory() const { return memory_; }

    inline bool isReady() { return isReady_; }

private:
    VkPhysicalDevice physicalDevice_;
    VkDevice logicalDevice_;
    uint32_t queueFamilyIndex_;
    VkDeviceSize size_;


    VkBuffer buffer_;
    VkDeviceMemory memory_;

    bool isReady_;
};

#endif //LIBRETRORUNNER_VK_READ_WRITE_BUFFER_H
