//
// Created by aidoo on 1/24/2025.
//
#include <vulkan/vulkan.h>
#include <android/native_window_jni.h>

#include "video_context_vulkan.h"
#include "vulkan_wrapper.h"

#include <libretro.h>
#include <libretro_vulkan.h>
#include <dlfcn.h>

#include "../../types/log.h"
#include "../../app/app_context.h"
#include "../../app/environment.h"
#include "../../app/setting.h"
#include "../../types/retro_types.h"

#include "rr_vulkan_instance.h"
#include "rr_vulkan_pipeline.h"
#include "rr_vulkan_swapchain.h"
#include "rr_draws.h"
#include "vk_sampling_texture.h"
#include "vk_read_write_buffer.h"

#define LOGD_VVC(...) LOGD("[Video] " __VA_ARGS__)
#define LOGW_VVC(...) LOGW("[Video] " __VA_ARGS__)
#define LOGE_VVC(...) LOGE("[Video] " __VA_ARGS__)
#define LOGI_VVC(...) LOGI("[Video] " __VA_ARGS__)

#define CHECK_VK_OBJ_NOT_NULL(arg, msg) if (arg == nullptr) { LOGE_VVC(msg); return false; }

void *getVKApiAddress(const char *sym) {
    void *libvulkan = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!libvulkan)
        return 0;
    return dlsym(libvulkan, sym);
}

namespace libRetroRunner {

    VulkanVideoContext::VulkanVideoContext() {
        InitVulkanApi();
        getHWProcAddress = (rr_hardware_render_proc_address_t) &getVKApiAddress;
        vulkanInstance_ = nullptr;
        vulkanPipeline_ = nullptr;
        width_ = 100;
        height_ = 100;
        vulkanIsReady_ = false;
    }

    VulkanVideoContext::~VulkanVideoContext() {

    }

    bool VulkanVideoContext::Load() {
        auto appContext = AppContext::Current();
        auto coreCtx = appContext->GetCoreRuntimeContext();
        auto gameCtx = appContext->GetGameRuntimeContext();
        core_pixel_format_ = coreCtx->GetPixelFormat();

        if (!InitVulkanApi()) {
            LOGE_VVC("Failed to init vulkan api.");
            return false;
        }
        if (vulkanInstance_ == nullptr) {
            vulkanInstance_ = new VulkanInstance();
            if (retroHWNegotiationInterface_) {
                vulkanInstance_->setRetroNegotiationInterface(retroHWNegotiationInterface_);
            }
            vulkanInstance_->setEnableLogger(true);
            if (!vulkanInstance_->init()) {
                vulkanInstance_->destroy();
                delete vulkanInstance_;
                vulkanInstance_ = nullptr;
                LOGE_VVC("Failed to init vulkan instance.");
                return false;
            }

        }
        CHECK_VK_OBJ_NOT_NULL(vulkanInstance_, "vulkan instance is null.")
        if (vulkanPipeline_ == nullptr) {
            vulkanPipeline_ = new VulkanPipeline(vulkanInstance_->getLogicalDevice());
            if (!vulkanPipeline_->init(width_, height_)) {
                delete vulkanPipeline_;
                vulkanPipeline_ = nullptr;
                LOGE_VVC("Failed to init vulkan pipeline.");
                return false;
            }
        }
        CHECK_VK_OBJ_NOT_NULL(vulkanPipeline_, "vulkan pipe line is null.")
        if (vulkanSwapchain_ == nullptr) {
            vulkanSwapchain_ = new VulkanSwapchain();
            vulkanSwapchain_->setInstance(vulkanInstance_->getInstance());
            vulkanSwapchain_->setPhysicalDevice(vulkanInstance_->getPhysicalDevice());
            vulkanSwapchain_->setLogicalDevice(vulkanInstance_->getLogicalDevice());
            vulkanSwapchain_->setQueueFamilyIndex(vulkanInstance_->getQueueFamilyIndex());
            vulkanSwapchain_->setRenderPass(vulkanPipeline_->getRenderPass());
            vulkanSwapchain_->setCommandPool(vulkanInstance_->getCommandPool());
            if (!vulkanSwapchain_->init(window_)) {
                delete vulkanSwapchain_;
                vulkanSwapchain_ = nullptr;
                LOGE_VVC("Failed to init vulkan swapchain.");
                return false;
            }
        }
        CHECK_VK_OBJ_NOT_NULL(vulkanSwapchain_, "vulkan swapchain is null.")
        vulkanIsReady_ = true;
        return true;
    }

    void VulkanVideoContext::Destroy() {

    }

    void VulkanVideoContext::Unload() {
        vulkanIsReady_ = false;
        if (vulkanSwapchain_) {
            vkQueueWaitIdle(vulkanInstance_->getGraphicQueue());
            vkDeviceWaitIdle(vulkanInstance_->getLogicalDevice());
            delete vulkanSwapchain_;
            vulkanSwapchain_ = nullptr;
        }

    }

    /*
    void VulkanVideoContext::SetSurface(int argc, void **argv) {
        JNIEnv *env = (JNIEnv *) argv[0];
        jobject surface = (jobject) argv[1];
        if (surface == nullptr) {
            window_ = nullptr;
            vulkanIsReady_ = false;
            if (vulkanInstance_ && vulkanSwapchain_) {
                vkQueueWaitIdle(vulkanInstance_->getGraphicQueue());
                vkDeviceWaitIdle(vulkanInstance_->getLogicalDevice());
                delete vulkanSwapchain_;
                vulkanSwapchain_ = nullptr;
            }
        } else {
            ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
            if (window == nullptr) {
                LOGE_VVC("Failed to get ANativeWindow from surface.");
                return;
            }
            window_ = window;
        }
    }

    void VulkanVideoContext::SetSurfaceSize(unsigned int width, unsigned int height) {
        if (width_ == width && height_ == height) {
            return;
        }
        width_ = width;
        height_ = height;

        if (vulkanIsReady_ && vulkanSwapchain_) {
            vulkanIsReady_ = false;
            vkQueueWaitIdle(vulkanInstance_->getGraphicQueue());
            vkDeviceWaitIdle(vulkanInstance_->getLogicalDevice());
            delete vulkanSwapchain_;
            vulkanSwapchain_ = nullptr;
            vkQueueWaitIdle(vulkanInstance_->getGraphicQueue());
            vkDeviceWaitIdle(vulkanInstance_->getLogicalDevice());
            Init();

        }
    }*/

    void VulkanVideoContext::Prepare() {

    }

    void VulkanVideoContext::OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) {
        if (data && data != RETRO_HW_FRAME_BUFFER_VALID && vulkanIsReady_) {
            if (softwareTexture_ == nullptr || !(softwareTexture_->getWidth() == width && softwareTexture_->getHeight() == height)) {
                if (softwareTexture_) {
                    delete softwareTexture_;
                }
                softwareTexture_ = new VulkanSamplingTexture(vulkanInstance_->getPhysicalDevice(), vulkanInstance_->getLogicalDevice(), vulkanInstance_->getQueueFamilyIndex());
                softwareTexture_->create(width, height);
            }
            softwareTexture_->update(vulkanInstance_->getGraphicQueue(), vulkanInstance_->getCommandPool(), data, pitch * height, core_pixel_format_);
            VkDescriptorSet descriptorSet = vulkanPipeline_->getDescriptorSet();
            if (descriptorSet)
                softwareTexture_->updateToDescriptorSet(descriptorSet);

        }
        if (data)
            DrawFrame();
    }

    void VulkanVideoContext::DrawFrame() {
        if (vulkanIsReady_ && softwareTexture_) {
            vulkanCommitFrame();
        }
    }

    unsigned int VulkanVideoContext::GetCurrentFramebuffer() {
        return 0;
    }

    bool VulkanVideoContext::TakeScreenshot(const std::string &path) {
        return false;
    }

    void VulkanVideoContext::vulkanCommitFrame() {

        uint32_t nextImageIndex = 0;
        VkSemaphore semaphore = vulkanSwapchain_->getSemaphore();
        VkFence fence = vulkanSwapchain_->getFence();
        VkDevice logicalDevice = vulkanInstance_->getLogicalDevice();
        VkSwapchainKHR swapchain = vulkanSwapchain_->getSwapchain();

        VkResult acquireImageResult = vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX, semaphore, fence, &nextImageIndex);
        VkCommandBuffer commandBuffer = vulkanSwapchain_->getCommandBuffer(nextImageIndex);
        vkResetCommandBuffer(commandBuffer, 0);

        recordCommandBufferForSoftwareRender(commandBuffer, nextImageIndex);

        if (acquireImageResult != VK_SUCCESS) {
            LOGE_VC("Failed to acquire next image!\n");
            return;
        }
        vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, UINT64_MAX);
        if (vkResetFences(logicalDevice, 1, &fence) != VK_SUCCESS) {
            LOGE_VC("Failed to reset fence!\n");
            return;
        }

        VkQueue queue = vulkanInstance_->getGraphicQueue();

        VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &semaphore,
                .pWaitDstStageMask = &waitStageMask,
                .commandBufferCount = 1,
                .pCommandBuffers = &commandBuffer,
                .signalSemaphoreCount = 0,
                .pSignalSemaphores = nullptr};
        VkResult submitResult = vkQueueSubmit(queue, 1, &submit_info, fence);
        if (submitResult != VK_SUCCESS) {
            LOGE_VC("Failed to submit queue: %d", submitResult);
            return;
        }

        VkResult result;
        VkPresentInfoKHR presentInfo{
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext = nullptr,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = nullptr,
                .swapchainCount = 1,
                .pSwapchains = &swapchain,
                .pImageIndices = &nextImageIndex,
                .pResults = &result,
        };
        vkQueuePresentKHR(queue, &presentInfo);
    }

    struct VObject {
        float x, y, z, w;  //position
        float vx, vy;      //texture coordinate
    };

    VObject vertices[] = {
            VObject{-1.0, -1.0, 0.0, 1.0, 0.0, 0.0},     //left-bottom
            VObject{1.0, -1.0, 0.0, 1.0, 1.0, 0.0},      // right-bottom
            VObject{1.0, 1.0, 0.0, 1.0, 1.0, 1.0},       //right-top
            VObject{-1.0, 1.0, 0.0, 1.0, 0.0, 1.0},     //left-top

            VObject{-1.0, -1.0, 0.0, 1.0, 0.0, 0.0},    //left-bottom
            VObject{1.0, -1.0, 0.0, 1.0, 1.0, 0.0},     // right-bottom
    };


    void VulkanVideoContext::recordCommandBufferForSoftwareRender(void *pCommandBuffer, uint32_t imageIndex) {
        if (vertexBuffer_ == nullptr) {
            vertexBuffer_ = new VulkanRWBuffer(vulkanInstance_->getPhysicalDevice(), vulkanInstance_->getLogicalDevice(), vulkanInstance_->getQueueFamilyIndex());
            vertexBuffer_->create(sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            vertexBuffer_->update(vertices, sizeof(vertices));
        }

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        VkCommandBuffer commandBuffer = (VkCommandBuffer) pCommandBuffer;

        VkCommandBufferBeginInfo beginInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        };
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkRenderPassBeginInfo renderPassInfo{};
        {
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = vulkanPipeline_->getRenderPass();
            renderPassInfo.framebuffer = vulkanSwapchain_->getFramebuffer(imageIndex);
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = vulkanSwapchain_->getExtent();

            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;
        }
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline_->getPipeline());

        VkViewport viewport{
                .width = (float) width_,
                .height = (float) height_,
                .minDepth = 0.0f,
                .maxDepth = 1.0f
        };
        VkRect2D scissor{
                .offset = {0, 0},
                .extent = {width_, width_}
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        //绑定顶点缓冲区
        VkDeviceSize offsets[] = {0};
        VkBuffer vertexBuffer = vertexBuffer_->getBuffer();
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

        //绑定描述符集（纹理）
        VkDescriptorSet descriptorSet = vulkanPipeline_->getDescriptorSet();
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline_->getLayout(),
                                0, 1, &descriptorSet, 0, nullptr);

        vkCmdDraw(commandBuffer, 6, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);
    }

    void VulkanVideoContext::setHWRenderContextNegotiationInterface(const void *interface) {
        if (!interface) return;
        auto baseInterface = static_cast<const retro_hw_render_context_negotiation_interface *>(interface);
        if (baseInterface->interface_type == RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN) {
            retroHWNegotiationInterface_ = static_cast<const retro_hw_render_context_negotiation_interface_vulkan *>(interface);
        }
    }

    void VulkanVideoContext::UpdateVideoSize(unsigned int width, unsigned int height) {

    }


}
