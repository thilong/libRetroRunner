//
// Created by aidoo.
//

#ifndef LIBRETRORUNNER_RR_DRAWS_H
#define LIBRETRORUNNER_RR_DRAWS_H

#include "rr_vulkan.h"

namespace libRetroRunner {
    class RRTestDrawCommand {

    };

    class VulkanSamplerTexture {
    public:
        VulkanSamplerTexture(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t queueFamilyIndex);

        bool create(uint32_t width, uint32_t height, VkFormat format);

        bool updateData(VkCommandPool commandPool, VkQueue queue, const void *data, uint32_t width, uint32_t height, size_t pitch, int pixelFormat);

        void updateDescriptorSet(VkDescriptorSet descriptorSet);

    private:
        VkDevice device_{};
        VkPhysicalDevice physicalDevice_{};
        uint32_t queueFamilyIndex_{};
        uint32_t width_{};
        uint32_t height_{};

        VkFormat format_{};
        VkImage image_{};
        VkImageView  imageView_{};
        VkDeviceMemory imageMemory_{};

        VkBuffer stagingBuffer{};
        VkDeviceMemory stagingBufferMemory{};


        VkSampler sampler_{};

        unsigned char *convertBuffer;
    };
}
#endif //LIBRETRORUNNER_RR_DRAWS_H
