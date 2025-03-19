//
// Created by aidoo.
//
#include "rr_vulkan_swapchain.h"

namespace libRetroRunner {
    VulkanSwapchain::VulkanSwapchain() {
        window_ = nullptr;
        allReady = false;
    }

    VulkanSwapchain::~VulkanSwapchain() {
        destroy();
    }

    bool VulkanSwapchain::createSurface() {
        VkAndroidSurfaceCreateInfoKHR createInfo{
                .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .window = (ANativeWindow *) window_
        };
        VkResult createRet = vkCreateAndroidSurfaceKHR(instance_, &createInfo, nullptr, &surface_);

        if (createRet != VK_SUCCESS || surface_ == VK_NULL_HANDLE) {
            LOGE_VC("Surface create failed:  %d\n", createRet);
            return false;
        }
        LOGD_VC("Surface:\t\t%p\n", surface_);

        return true;
    }

    bool VulkanSwapchain::createSwapchain() {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkResult surfaceCapabilityRet = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &surfaceCapabilities);
        if (surfaceCapabilityRet != VK_SUCCESS) {
            LOGE_VC("Can't get capability from surface.");
            return false;
        }
        uint32_t surfaceFormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &surfaceFormatCount, nullptr);
        if (surfaceFormatCount < 1) {
            LOGE_VC("No surface format found.");
            return false;
        }
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &surfaceFormatCount, surfaceFormats.data())) {
            LOGE_VC("Failed to get formats of surface.");
            return false;
        }

        uint32_t chosenIndex = 0;
        for (; chosenIndex < surfaceFormatCount; chosenIndex++) {
            if (surfaceFormats[chosenIndex].format == VK_FORMAT_R8G8B8A8_UNORM)
                break;
        }
        if (chosenIndex >= surfaceFormatCount) {
            LOGE_VC("No suitable surface format found.");
            return false;
        }
        extent_ = surfaceCapabilities.currentExtent;
        format_ = surfaceFormats[chosenIndex].format;
        colorSpace_ = surfaceFormats[chosenIndex].colorSpace;

        LOGD_VC("Surface chosen: [ %d x %d ], format: %d, color space: %d\n", extent_.width, extent_.height, format_, colorSpace_);

        VkSwapchainCreateInfoKHR swapchainCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = nullptr,
                .surface = surface_,
                .minImageCount = surfaceCapabilities.minImageCount,
                .imageFormat = format_,
                .imageColorSpace = colorSpace_,
                .imageExtent = surfaceCapabilities.currentExtent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &queueFamilyIndex_,
                .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, //android: VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,  windows: VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
                .presentMode = VK_PRESENT_MODE_FIFO_KHR,
                .clipped = VK_FALSE,
                .oldSwapchain = VK_NULL_HANDLE, //注意这里，是否要在下一次创建的时候释放掉之前的swapchain
        };
        VkResult createSwapChainResult = vkCreateSwapchainKHR(logicalDevice_, &swapchainCreateInfo, nullptr, &swapchain_);
        if (createSwapChainResult || swapchain_ == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create swap chain! %d\n", createSwapChainResult);
            return false;
        } else {
            LOGD_VC("swapchain:\t\t%p\n", swapchain_);
        }

        vkGetSwapchainImagesKHR(logicalDevice_, swapchain_, &imageCount_, nullptr);
        LOGD_VC("Swapchain image count: %d\n", imageCount_);

        return true;
    }

    bool VulkanSwapchain::createImageViews() {
        if (imageCount_ < 1) {
            LOGE_VC("Swapchain do not have any image.\n");
            return false;
        }
        images_.resize(imageCount_);
        vkGetSwapchainImagesKHR(logicalDevice_, swapchain_, &imageCount_, images_.data());
        imageViews_.resize(imageCount_);
        for (uint32_t imgIdx = 0; imgIdx < imageCount_; imgIdx++) {
            VkImageViewCreateInfo viewCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .image = images_[imgIdx],
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = format_,
                    .components{
                            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                    .subresourceRange{
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = 1,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                    },
            };
            if (vkCreateImageView(logicalDevice_, &viewCreateInfo, nullptr, &imageViews_[imgIdx]) != VK_SUCCESS || imageViews_[imgIdx] == VK_NULL_HANDLE) {
                LOGE_VC("Failed to create swapchain image view [%d]!\n", imgIdx);
                return false;
            }
        }
        LOGD_VC("Swapchain image view created: %d\n", imageCount_);
        return true;
    }

    bool VulkanSwapchain::createFrameBuffers() {
        frameBuffers_.resize(imageCount_);
        for (int idx = 0; idx < imageCount_; idx++) {
            VkImageView attachmentViews[] = {imageViews_[idx]};
            VkFramebufferCreateInfo fbCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .pNext = nullptr,
                    .renderPass = renderPass_,
                    .attachmentCount = 1, // TODO: (depthView == VK_NULL_HANDLE ? 1 : 2),
                    .pAttachments = attachmentViews,
                    .width = static_cast<uint32_t>(extent_.width),
                    .height = static_cast<uint32_t>(extent_.height),
                    .layers = 1,
            };
            if (vkCreateFramebuffer(logicalDevice_, &fbCreateInfo, nullptr, &frameBuffers_[idx]) != VK_SUCCESS || frameBuffers_[idx] == VK_NULL_HANDLE) {
                LOGE_VC("Failed to create frame buffer %d!\n", idx);
                return false;
            }
        }
        LOGD_VC("Frame buffers created: total %d\n", imageCount_);
        return true;
    }

    bool VulkanSwapchain::createCommandBuffers() {
        commandBuffers_.resize(imageCount_);
        VkCommandBufferAllocateInfo cmdBufferCreateInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = commandPool_,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = imageCount_,
        };
        VkResult createCmdBufferResult = vkAllocateCommandBuffers(logicalDevice_, &cmdBufferCreateInfo, commandBuffers_.data());
        if (createCmdBufferResult != VK_SUCCESS) {
            LOGE_VC("Failed to create command buffers! %d\n", createCmdBufferResult);
            return false;
        }

        return true;
    }

    bool VulkanSwapchain::createSyncObjects() {
        VkFenceCreateInfo fenceCreateInfo{
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
        };

        VkResult createFenceResult = vkCreateFence(logicalDevice_, &fenceCreateInfo, nullptr, &fence_);

        if (createFenceResult != VK_SUCCESS || fence_ == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create fence! %d\n", createFenceResult);
            return false;
        }

        // We need to create a semaphore to be able to wait, in the main loop, for our
        // framebuffer to be available for us before drawing.
        VkSemaphoreCreateInfo semaphoreCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
        };
        VkResult createSemaphoreResult = vkCreateSemaphore(logicalDevice_, &semaphoreCreateInfo, nullptr, &semaphore_);
        if (createSemaphoreResult != VK_SUCCESS || semaphore_ == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create semaphore! %d", createSemaphoreResult);
            return false;
        }
        LOGD_VC("semaphore:\t\t%p\n", semaphore_);
        LOGD_VC("fence:\t\t\t%p\n", fence_);
        return true;
    }

    bool VulkanSwapchain::init(void *window) {
        window_ = window;
        do {
            if (!window_) {
                LOGE_VC("Window for swapchain is null.");
                break;
            }
            if (!instance_) {
                LOGE_VC("Vulkan instance is not ready for swapchain.");
                break;
            }
            if (!physicalDevice_) {
                LOGE_VC("Physical device is not ready for swapchain.");
                break;
            }
            if (!logicalDevice_) {
                LOGE_VC("Logical device is not ready for swapchain.");
                break;
            }
            if (!renderPass_) {
                LOGE_VC("Render pass is not ready for swapchain.");
                break;
            }
            if (!commandPool_) {
                LOGE_VC("Command pool is not ready for swapchain.");
                break;
            }

            if (!createSurface()) break;
            if (!createSwapchain()) break;
            if (!createImageViews()) break;
            if (!createFrameBuffers()) break;
            if (!createCommandBuffers()) break;
            if (!createSyncObjects()) break;
            allReady = true;
        } while (false);
        if (!allReady) {
            destroy();
        }
        return allReady;
    }

    void VulkanSwapchain::destroy() {
        if (fence_) {
            vkDestroyFence(logicalDevice_, fence_, nullptr);
            fence_ = VK_NULL_HANDLE;
        }
        if (semaphore_) {
            vkDestroySemaphore(logicalDevice_, semaphore_, nullptr);
            semaphore_ = VK_NULL_HANDLE;
        }
        for (auto frameBuffer: frameBuffers_) {
            vkDestroyFramebuffer(logicalDevice_, frameBuffer, nullptr);
        }
        if (commandPool_) {
            vkDestroyCommandPool(logicalDevice_, commandPool_, nullptr);
        }

        for (auto imageView: imageViews_) {
            vkDestroyImageView(logicalDevice_, imageView, nullptr);
        }
        imageViews_.resize(0);
        images_.resize(0);
        imageCount_ = 0;
        if (swapchain_) {
            vkDestroySwapchainKHR(logicalDevice_, swapchain_, nullptr);
            swapchain_ = VK_NULL_HANDLE;
        }
        if (surface_) {
            vkDestroySurfaceKHR(instance_, surface_, nullptr);
            surface_ = VK_NULL_HANDLE;
        }
        allReady = false;
    }

}


