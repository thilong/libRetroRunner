//
// Created by aidoo.
//

#ifndef LIBRETRORUNNER_RR_VULKAN_SWAPCHAIN_H
#define LIBRETRORUNNER_RR_VULKAN_SWAPCHAIN_H

#include "rr_vulkan.h"

namespace libRetroRunner {
    class VulkanSwapchain {
    public:

        VulkanSwapchain();

        ~VulkanSwapchain();


    public:

        bool createSurface();

        bool createSwapchain();

        bool createImageViews();

        bool createFrameBuffers();

        bool createCommandBuffers();

        bool createSyncObjects();

        bool init(void *window);

        void destroy();

    public:

        VkSurfaceKHR getSurface() const { return surface_; }

        VkSwapchainKHR getSwapchain() const { return swapchain_; }

        VkFormat getFormat() const { return format_; }

        VkExtent2D getExtent() const { return extent_; }

        VkColorSpaceKHR getColorSpace() const { return colorSpace_; }

        uint32_t getImageCount() const { return imageCount_; }
        
        const std::vector<VkCommandBuffer> &getCommandBuffers() const { return commandBuffers_; }

        VkSemaphore getSemaphore() const { return semaphore_; }

        VkFence getFence() const { return fence_; }

        VkCommandBuffer getCommandBuffer(uint32_t idx) {
            if (idx >= commandBuffers_.size()) return VK_NULL_HANDLE;
            return commandBuffers_[idx];
        }

        VkFramebuffer getFramebuffer(uint32_t idx) {
            if (idx >= frameBuffers_.size()) return VK_NULL_HANDLE;
            return frameBuffers_[idx];
        }

        VkImage getImage(uint32_t idx) {
            if (idx >= images_.size()) return VK_NULL_HANDLE;
            return images_[idx];
        }

    public:

        void setInstance(VkInstance instance) { instance_ = instance; }

        void setPhysicalDevice(VkPhysicalDevice physicalDevice) { physicalDevice_ = physicalDevice; }

        void setLogicalDevice(VkDevice logicalDevice) { logicalDevice_ = logicalDevice; }

        void setQueueFamilyIndex(uint32_t queueFamilyIndex) { queueFamilyIndex_ = queueFamilyIndex; }

        void setRenderPass(VkRenderPass renderPass) { renderPass_ = renderPass; }

        void setCommandPool(VkCommandPool commandPool) { commandPool_ = commandPool; }

    private:
        VkInstance instance_ = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
        VkDevice logicalDevice_ = VK_NULL_HANDLE;
        uint32_t queueFamilyIndex_ = 0;
        VkRenderPass renderPass_ = VK_NULL_HANDLE;
        VkCommandPool commandPool_ = VK_NULL_HANDLE;

        VkSurfaceKHR surface_ = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
        VkFormat format_{};
        VkExtent2D extent_ = {0, 0};
        VkColorSpaceKHR colorSpace_{};

        uint32_t imageCount_ = 0;

        std::vector<VkImage> images_{};
        std::vector<VkImageView> imageViews_{};
        std::vector<VkFramebuffer> frameBuffers_{};


        std::vector<VkCommandBuffer> commandBuffers_{};

        VkSemaphore semaphore_ = VK_NULL_HANDLE;
        VkFence fence_ = VK_NULL_HANDLE;

        void *window_ = nullptr;
        bool allReady = false;
    };
}


#endif //LIBRETRORUNNER_RR_VULKAN_SWAPCHAIN_H
