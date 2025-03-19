//
// Created by aidoo.
//

#include "vk_sampling_texture.h"
#include "vk_read_write_buffer.h"

VulkanSamplingTexture::VulkanSamplingTexture(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, uint32_t queueFamilyIndex) :
        physicalDevice_(physicalDevice), logicalDevice_(logicalDevice), queueFamilyIndex_(queueFamilyIndex),
        width_(0), height_(0), image_(VK_NULL_HANDLE), imageView_(VK_NULL_HANDLE), memory_(VK_NULL_HANDLE), sampler_(VK_NULL_HANDLE),
        stagingBuffer_(nullptr), convertBuffer(nullptr) {}

VulkanSamplingTexture::~VulkanSamplingTexture() {
    destroy();
}

bool VulkanSamplingTexture::create(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    VkImageCreateInfo imageInfo{};
    {
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {width_, height_, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    VkResult result = vkCreateImage(logicalDevice_, &imageInfo, nullptr, &image_);
    if (result != VK_SUCCESS) {
        return false;
    }
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(logicalDevice_, image_, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    {
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VkUtil::findMemoryType(physicalDevice_, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
    vkAllocateMemory(logicalDevice_, &allocInfo, nullptr, &memory_);
    vkBindImageMemory(logicalDevice_, image_, memory_, 0);

    VkImageViewCreateInfo imageViewCreateInfo{};
    {
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = image_;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
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
    result = vkCreateImageView(logicalDevice_, &imageViewCreateInfo, nullptr, &imageView_);
    if (result != VK_SUCCESS) {
        LOGE_VC("Failed to create image view: %d", result);
        return false;
    }

    //放大和缩小时使用的插值方式 VK_FILTER_NEAREST: 临近， VK_FILTER_LINEAR: 线性
    VkFilter filter = VK_FILTER_NEAREST;
    VkSamplerCreateInfo samplerInfo{};
    {
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = filter;
        samplerInfo.minFilter = filter;
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
    }
    VkResult samplerResult = vkCreateSampler(logicalDevice_, &samplerInfo, nullptr, &sampler_);
    if (samplerResult != VK_SUCCESS) {
        LOGE_VC("failed to create texture sampler: %d", samplerResult);
        return false;
    }
    return true;
}

bool VulkanSamplingTexture::update(VkQueue queue, VkCommandPool commandPool, const void *data, size_t size, int pixelFormat) {
    VkDeviceSize imageSize = height_ * height_ * 4;  //for R8G8B8A8
    if (!stagingBuffer_) {
        stagingBuffer_ = new VulkanRWBuffer(physicalDevice_, logicalDevice_, queueFamilyIndex_);
    }
    if (!stagingBuffer_->isReady()) {
        if (!stagingBuffer_->create(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) {
            return false;
        }
    }
    if (pixelFormat == PIXEL_FORMAT_RGB565) {
        if (!convertBuffer) {
            convertBuffer = new unsigned char[imageSize];
        }
        VkUtil::convertRGB565ToRGBA8888(data, width_, height_, convertBuffer);
        stagingBuffer_->update(convertBuffer, imageSize);
    } else {
        stagingBuffer_->update(data, size);
    }

    VkCommandBuffer commandBuffer = VkUtil::beginSingleTimeCommands(logicalDevice_, commandPool);

    // 转换图像布局为 `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`
    VkUtil::transitionImageLayout(logicalDevice_, commandPool, queue, image_, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // 复制 Staging Buffer 到 Image
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {width_, height_, 1};

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer_->getBuffer(), image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    //转换图像布局回 `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
    VkUtil::transitionImageLayout(logicalDevice_, commandPool, queue, image_, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkUtil::endSingleTimeCommands(logicalDevice_, commandPool, queue, commandBuffer);
    return true;
}

void VulkanSamplingTexture::destroy() {
    if (imageView_) {
        vkDestroyImageView(logicalDevice_, imageView_, nullptr);
        imageView_ = VK_NULL_HANDLE;
    }
    if (image_) {
        vkDestroyImage(logicalDevice_, image_, nullptr);
        image_ = VK_NULL_HANDLE;
    }
    if (memory_) {
        vkFreeMemory(logicalDevice_, memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }
    if (sampler_) {
        vkDestroySampler(logicalDevice_, sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }
    if (stagingBuffer_) {
        delete stagingBuffer_;
        stagingBuffer_ = nullptr;
    }
    if (convertBuffer) {
        delete[] convertBuffer;
        convertBuffer = nullptr;
    }
}

void VulkanSamplingTexture::updateToDescriptorSet(VkDescriptorSet descriptorSet) {
    VkUtil::updateDescriptorSet(logicalDevice_, descriptorSet, imageView_, sampler_);
}
