//
// Created by aidoo.
//

#ifndef LIBRETRORUNNER_VK_SAMPLING_TEXTURE_H
#define LIBRETRORUNNER_VK_SAMPLING_TEXTURE_H

#include "rr_vulkan.h"

/* Helper class, always in VK_FORMAT_R8G8B8A8_UNORM format*/
class VulkanSamplingTexture {
public:
    VulkanSamplingTexture(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, uint32_t queueFamilyIndex);

    ~VulkanSamplingTexture();

    bool create(uint32_t width, uint32_t height);

    bool update(VkQueue queue, VkCommandPool commandPool, const void *data, size_t size, int pixelFormat);

    void destroy();

    void updateToDescriptorSet(VkDescriptorSet descriptorSet);


public:
    inline VkSampler getSampler() const { return sampler_; }

    inline VkImageView getImageView() const { return imageView_; }

    inline VkImage getImage() const { return image_; }

    inline uint32_t getWidth() const { return width_; }

    inline uint32_t getHeight() const { return height_; }

private:
    VkPhysicalDevice physicalDevice_;
    VkDevice logicalDevice_;
    uint32_t queueFamilyIndex_;

    uint32_t width_;
    uint32_t height_;

    VkImage image_;
    VkImageView imageView_;
    VkDeviceMemory memory_;

    VkSampler sampler_;

    class VulkanRWBuffer *stagingBuffer_;

    unsigned char *convertBuffer;
};

#endif
