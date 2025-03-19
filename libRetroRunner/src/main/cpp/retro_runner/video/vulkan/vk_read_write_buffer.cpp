//
// Created by aidoo.
//

#include "vk_read_write_buffer.h"

VulkanRWBuffer::VulkanRWBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, uint32_t queueFamilyIndex) :
        physicalDevice_(physicalDevice),
        logicalDevice_(logicalDevice),
        queueFamilyIndex_(queueFamilyIndex),
        isReady_(false) {
}

VulkanRWBuffer::~VulkanRWBuffer() {
    destroy();
}

bool VulkanRWBuffer::create(VkDeviceSize size, VkBufferUsageFlags usage) {
    VkBufferCreateInfo createBufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &queueFamilyIndex_
    };
    VkResult result = vkCreateBuffer(logicalDevice_, &createBufferInfo, nullptr, (VkBuffer *) &buffer_);
    if (result != VK_SUCCESS) {
        LOGE_VC("failed to create buffer, error %d", result);
        return false;
    }
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logicalDevice_, (VkBuffer) buffer_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    VkUtil::MapMemoryTypeToIndex(
            physicalDevice_,
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &allocInfo.memoryTypeIndex);
    result = vkAllocateMemory(logicalDevice_, &allocInfo, nullptr, (VkDeviceMemory *) &memory_);
    if (result != VK_SUCCESS) {
        LOGE_VC("failed to allocate buffer memory, error %d", result);
        return false;
    }
    size_ = allocInfo.allocationSize;
    vkBindBufferMemory(logicalDevice_, (VkBuffer) buffer_, (VkDeviceMemory) memory_, 0);
    isReady_ = true;
    return true;
}

void VulkanRWBuffer::destroy() {
    if (memory_) {
        vkFreeMemory(logicalDevice_, memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }
    if (buffer_) {
        vkDestroyBuffer(logicalDevice_, buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
    }
    size_ = 0;
}

void VulkanRWBuffer::update(const void *data, VkDeviceSize size) {
    void *toData;
    vkMapMemory(logicalDevice_, memory_, 0, size_, 0, &toData);
    memcpy(toData, data, size > size_ ? size_ : size);
    vkUnmapMemory(logicalDevice_, memory_);
}
