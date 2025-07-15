//
// Created by aidoo.
//

#include "rr_draws.h"
#include "rr_vulkan.h"
#include <libretro.h>

namespace libRetroRunner {

    extern void convertRGB565ToRGBA8888(const void *data, unsigned int width, unsigned int height, unsigned char *buffer);

    VulkanSamplerTexture::VulkanSamplerTexture(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t queueFamilyIndex) {
        physicalDevice_ = physicalDevice;
        device_ = device;
        queueFamilyIndex_ = queueFamilyIndex;
    }

    bool VulkanSamplerTexture::create(uint32_t width, uint32_t height, VkFormat format) {
        format_ = format;
        convertBuffer = new unsigned char[width * height * 4];

        VkImageCreateInfo imageInfo{};
        {
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent = {width, height, 1};
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        if (vkCreateImage(device_, &imageInfo, nullptr, &image_) != VK_SUCCESS) {
            return false;
        }
        layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device_, image_, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        {
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = VkUtil::findMemoryType(physicalDevice_, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }
        vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory_);
        vkBindImageMemory(device_, image_, imageMemory_, 0);

        VkImageViewCreateInfo imageViewCreateInfo{};
        {
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = image_;
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = format_;
            imageViewCreateInfo.components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            };
            imageViewCreateInfo.subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            };
        }
        VkResult imageViewCreateResult = vkCreateImageView(device_, &imageViewCreateInfo, nullptr, &imageView_);
        if (imageViewCreateResult != VK_SUCCESS) {
            LOGE_VC("Failed to create image view: %d", imageViewCreateResult);
            return false;
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;  // 放大时使用线性插值
        samplerInfo.minFilter = VK_FILTER_LINEAR;  // 缩小时使用线性插值
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16.0f;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VkResult samplerResult = vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_);
        if (samplerResult != VK_SUCCESS) {
            LOGE_VC("failed to create texture sampler: %d", samplerResult);
            return false;
        }

        return true;
    }

    bool VulkanSamplerTexture::updateData(VkCommandPool commandPool, VkQueue queue, const void *data, uint32_t width, uint32_t height, size_t pitch, int pixelFormat) {
        VkDeviceSize imageSize = width * height * 4;

        // 1. 创建 Staging Buffer
        if (stagingBuffer == VK_NULL_HANDLE) {
            VkUtil::createBuffer(physicalDevice_, device_, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 stagingBuffer, stagingBufferMemory);
        }

        // 2. 更新 Staging Buffer 数据
        void *mappedData;
        vkMapMemory(device_, stagingBufferMemory, 0, imageSize, 0, &mappedData);
        if (pixelFormat == RETRO_PIXEL_FORMAT_RGB565) {
            convertRGB565ToRGBA8888(data, width, height, convertBuffer);
            memcpy(mappedData, convertBuffer, (size_t) imageSize);
        } else {
            memcpy(mappedData, data, (size_t) imageSize);
        }
        vkUnmapMemory(device_, stagingBufferMemory);
        // 3. 记录命令缓冲区
        VkCommandBuffer commandBuffer = VkUtil::beginSingleTimeCommands(device_, commandPool);

        // 4. 转换图像布局为 `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`
        VkUtil::transitionImageLayout(device_, commandPool, queue, image_, format_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &layout_);

        // 5. 复制 Staging Buffer 到 Image
        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // 6. 转换图像布局回 `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
        VkUtil::transitionImageLayout(device_, commandPool, queue, image_, format_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &layout_);

        // 7. 提交命令
        VkUtil::endSingleTimeCommands(device_, commandPool, queue, commandBuffer);
        return true;
    }

    void VulkanSamplerTexture::updateDescriptorSet(VkDescriptorSet descriptorSet) {
        VkUtil::updateDescriptorSet(device_, descriptorSet, imageView_, sampler_);
    }


}
