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

#include "shader_code_frag.h"
#include "shader_code_vert.h"


#define LOGD_VVC(...) LOGD("[Vulkan] " __VA_ARGS__)
#define LOGW_VVC(...) LOGW("[Vulkan] " __VA_ARGS__)
#define LOGE_VVC(...) LOGE("[Vulkan] " __VA_ARGS__)
#define LOGI_VVC(...) LOGI("[Vulkan] " __VA_ARGS__)

#define DRAW_LOGD_VVC(...) LOGD("[Vulkan] " __VA_ARGS__)

#define CHECK_VK_OBJ_NOT_NULL(arg, msg) if (arg == nullptr) { LOGE_VVC(msg); return false; }

#define VK_BIT_TEST(flag, bit) (((flag) & (bit)) == (bit))
#define VK_BIT_SET(flag, bit) ((flag) |= (bit))
#define VK_BIT_CLEAR(flag, bit) ((flag) &= ~(bit))
#define min(a, b) ((a) < (b) ? (a) : (b))


extern rr_hardware_render_proc_address_t getHWProcAddress;

void *getVKApiAddress(const char *sym) {
    void *libvulkan = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!libvulkan)
        return 0;
    return dlsym(libvulkan, sym);
}

/** Declares */
namespace libRetroRunner {
    struct VObject {
        float x, y, z, w;  //position
        float vx, vy;      //texture coordinate
    };

    // texture coordinates and positions for a full-screen quad,  this is different from OPENGL ES
    VObject vertices[] = {
            VObject{-1.0, -1.0, 0.0, 1.0, 1.0, 0.0},     //left-bottom
            VObject{1.0, -1.0, 0.0, 1.0, 1.0, 1.0},      // right-bottom
            VObject{1.0, 1.0, 0.0, 1.0, 0.0, 1.0},       //right-top
            VObject{-1.0, 1.0, 0.0, 1.0, 0.0, 0.0},     //left-top

            VObject{-1.0, -1.0, 0.0, 1.0, 1.0, 0.0},    //left-bottom
            VObject{1.0, -1.0, 0.0, 1.0, 1.0, 1.0},     // right-bottom
    };

    VObject verticesOpenGL[] = {
            VObject{-1.0, -1.0, 0.0, 1.0, 0.0, 0.0},     //left-bottom
            VObject{1.0, -1.0, 0.0, 1.0, 1.0, 0.0},      // right-bottom
            VObject{1.0, 1.0, 0.0, 1.0, 1.0, 1.0},       //right-top
            VObject{-1.0, 1.0, 0.0, 1.0, 0.0, 1.0},     //left-top

            VObject{-1.0, -1.0, 0.0, 1.0, 0.0, 0.0},    //left-bottom
            VObject{1.0, -1.0, 0.0, 1.0, 1.0, 0.0},     // right-bottom
    };

}

/** misc functions */
namespace libRetroRunner {
    bool isVkPhysicalDeviceSuitable(VkPhysicalDevice device, int *graphicsFamily);

    bool vkFindDeviceExtensions(VkPhysicalDevice gpu, std::vector<const char *> &enabledExts, std::vector<const char *> &optionalExts);
}

/** negotiation interface implementation */
namespace libRetroRunner {
    static VkInstance retro_vulkan_create_instance_wrapper_t_impl(void *opaque, const VkInstanceCreateInfo *create_info) {
        if (opaque) {
            return ((VulkanVideoContext *) opaque)->retro_vulkan_create_instance_wrapper(create_info);
        }
        return VK_NULL_HANDLE;
    }

    static VkDevice retro_vulkan_create_device_wrapper_t_impl(VkPhysicalDevice gpu, void *opaque, const VkDeviceCreateInfo *create_info) {
        if (opaque) {
            return ((VulkanVideoContext *) opaque)->retro_vulkan_create_device_wrapper(gpu, create_info);
        }
        return VK_NULL_HANDLE;
    }

}

/** hardware rendering interface implementation */
namespace libRetroRunner {

    //TODO: 当前没有处理核心传出的信号量，当其不为空时，需要在绘制的时候处理
    void VulkanVideoContext::retro_vulkan_set_image_t_impl(const struct retro_vulkan_image *image, uint32_t num_semaphores, const VkSemaphore *semaphores, uint32_t src_queue_family) {
        negotiationImage_ = image;
        if (!negotiationSemaphores_) {
            negotiationSemaphores_ = new VkSemaphore[20];
        }
        if (!negotiationWaitStages_) {
            negotiationWaitStages_ = new VkPipelineStageFlags[20];
        }
        negotiationSemaphoreCount_ = num_semaphores;
        if (semaphores) {
            for (uint32_t i = 0; i < num_semaphores; ++i) {
                if (i > 19) {
                    LOGE_VVC("semaphores count %u is too large, max is 20", num_semaphores);
                    break;
                }
                negotiationSemaphores_[i] = semaphores[i];
                negotiationWaitStages_[i] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
        }
        negotiationQueueFamily_ = src_queue_family;

        //TODO: check line:3080 vk->flags |= VK_FLAG_HW_VALID_SEMAPHORE;
        if (image && image->image_view) {
            videoContentNeedUpdate_ = true;
            //LOGW_VVC("HW callback, frame: %lu, retro_vulkan_set_image_t_impl, image view: %p, layout: %d, flags: %d, semaphore count: %u, semaphore: %p, queue family: %u", frameCount_, image->image_view, image->image_layout, image->create_info.flags, num_semaphores, semaphores, src_queue_family);
        } else {
            //LOGW_VVC("HW callback, frame: %lu, retro_vulkan_set_image_t_impl empty: %p, %u, %p, %u", frameCount_, image, num_semaphores, semaphores, src_queue_family);
        }
    }

    uint32_t VulkanVideoContext::retro_vulkan_get_sync_index_t_impl() const {
        uint32_t ret = renderContext_.current_frame;
        //LOGW_VVC("frame: %lu, call retro_vulkan_get_sync_index_t_impl : %u", frameCount_, ret);
        return ret;
    }

    uint32_t VulkanVideoContext::retro_vulkan_get_sync_index_mask_t_impl() const {
        uint32_t mask = (1 << framesInFlight_) - 1;
        LOGW_VVC("frame: %lu, retro_vulkan_get_sync_index_mask_t_impl : %u", frameCount_, mask);
        return mask;
    }

    void VulkanVideoContext::retro_vulkan_set_command_buffers_t_impl(uint32_t num_cmd, const VkCommandBuffer *cmd) const {
        LOGW_VVC("frame: %lu, retro_vulkan_set_command_buffers_t_impl", frameCount_);
    }

    void VulkanVideoContext::retro_vulkan_wait_sync_index_t_impl() {
        //LOGW_VVC("need to implement retro_vulkan_wait_sync_index_t_impl");
    }

    void VulkanVideoContext::retro_vulkan_lock_queue_t_impl() {
        pthread_mutex_lock(queue_lock);
    }

    void VulkanVideoContext::retro_vulkan_unlock_queue_t_impl() {
        //LOGW_VVC("need to implement retro_vulkan_unlock_queue_t_impl");
        pthread_mutex_unlock(queue_lock);
    }

    void VulkanVideoContext::retro_vulkan_set_signal_semaphore_t_impl(VkSemaphore semaphore) {
        LOGW_VVC("frame: %lu, retro_vulkan_set_signal_semaphore_t_impl", frameCount_);
        negotiationSemaphore_ = semaphore;
    }
}

/** retro hardware render interface implementation wrapper*/
namespace libRetroRunner {

    void retro_vulkan_set_image_t_impl_wrapper(void *handle, const struct retro_vulkan_image *image, uint32_t num_semaphores, const VkSemaphore *semaphores, uint32_t src_queue_family) {
        if (handle) {
            ((VulkanVideoContext *) handle)->retro_vulkan_set_image_t_impl(image, num_semaphores, semaphores, src_queue_family);
        } else {
            LOGE_VVC("can't find retro_vulkan_set_image_t_impl on a null handle");
        }
    }

    uint32_t retro_vulkan_get_sync_index_t_impl_wrapper(void *handle) {
        if (handle) {
            return ((VulkanVideoContext *) handle)->retro_vulkan_get_sync_index_t_impl();
        } else {
            LOGE_VVC("can't find retro_vulkan_get_sync_index_t_impl on a null handle");
        }
        return 0;
    }

    uint32_t retro_vulkan_get_sync_index_mask_t_impl_wrapper(void *handle) {
        if (handle) {
            return ((VulkanVideoContext *) handle)->retro_vulkan_get_sync_index_mask_t_impl();
        } else {
            LOGE_VVC("can't find retro_vulkan_get_sync_index_mask_t_impl on a null handle");
        }
        return 0;
    }

    void retro_vulkan_set_command_buffers_t_impl_wrapper(void *handle, uint32_t num_cmd, const VkCommandBuffer *cmd) {
        if (handle) {
            return ((VulkanVideoContext *) handle)->retro_vulkan_set_command_buffers_t_impl(num_cmd, cmd);
        } else {
            LOGE_VVC("can't find retro_vulkan_set_command_buffers_t_impl on a null handle");
        }

    }

    void retro_vulkan_wait_sync_index_t_impl_wrapper(void *handle) {
        if (handle) {
            ((VulkanVideoContext *) handle)->retro_vulkan_wait_sync_index_t_impl();
        } else {
            LOGE_VVC("can't find retro_vulkan_wait_sync_index_t_impl on a null handle");
        }
    }

    void retro_vulkan_lock_queue_t_impl_wrapper(void *handle) {
        if (handle) {
            ((VulkanVideoContext *) handle)->retro_vulkan_lock_queue_t_impl();
        } else {
            LOGE_VVC("can't find retro_vulkan_lock_queue_t_impl on a null handle");
        }

    }

    void retro_vulkan_unlock_queue_t_impl_wrapper(void *handle) {
        if (handle) {
            ((VulkanVideoContext *) handle)->retro_vulkan_unlock_queue_t_impl();
        } else {
            LOGE_VVC("can't find retro_vulkan_unlock_queue_t_impl on a null handle");
        }

    }

    void retro_vulkan_set_signal_semaphore_t_impl_wrapper(void *handle, VkSemaphore semaphore) {
        if (handle) {
            ((VulkanVideoContext *) handle)->retro_vulkan_set_signal_semaphore_t_impl(semaphore);
        } else {
            LOGE_VVC("can't find retro_vulkan_set_signal_semaphore_t_impl on a null handle");
        }
    }
}

namespace libRetroRunner {

    VulkanVideoContext::VulkanVideoContext() {
        InitVulkanApi();
        getHWProcAddress = (rr_hardware_render_proc_address_t) &getVKApiAddress;

        is_vulkan_debug_ = true;

        screen_width_ = 1;
        screen_height_ = 100;

        retro_render_interface_ = nullptr;
        core_pixel_format_ = RETRO_PIXEL_FORMAT_XRGB8888;

        window_ = nullptr;

        instance_ = VK_NULL_HANDLE;
        physicalDevice_ = VK_NULL_HANDLE;
        logicalDevice_ = VK_NULL_HANDLE;

        destroyDeviceImpl_ = nullptr;
        negotiationSemaphores_ = nullptr;
        negotiationWaitStages_ = nullptr;
        queue_lock = new pthread_mutex_t;
        pthread_mutex_init(queue_lock, nullptr);
    }

    VulkanVideoContext::~VulkanVideoContext() {
        Destroy();
    }

    bool VulkanVideoContext::Load() {
        auto appContext = AppContext::Current();
        auto coreCtx = appContext->GetCoreRuntimeContext();
        auto gameCtx = appContext->GetGameRuntimeContext();
        core_pixel_format_ = coreCtx->GetPixelFormat();

        //加载Vulkan库
        if (vkGetInstanceProcAddr == nullptr) {
            if (!InitVulkanApi()) {
                LOGE_VVC("Failed to init vulkan api.");
                return false;
            }
            if (!vkGetInstanceProcAddr) {
                LOGE_VVC("Failed to load vkGetInstanceProcAddr symbol, vulkan library load failed?");
                return false;
            }
        }

        //检查核心提供的上下文协商的类型与版本是否符合要求
        const retro_hw_render_context_negotiation_interface_vulkan *negotiator = getNegotiationInterface();
        if (negotiator) {
            if (negotiator->interface_type != RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN) {
                LOGE_VVC("Got hardware context negotiation interface, but it's the wrong API.");
                return false;
            }
            if (negotiator->interface_version == 0) {
                LOGE_VVC("Got hardware context negotiation interface, but it's the wrong interface version.");
                return false;
            }
        }

        //创建 Vulkan Instance
        if (!vulkanCreateInstanceIfNeeded()) return false;

        //创建 surface
        if (!vulkanCreateSurfaceIfNeeded()) return false;

        //创建显示设备
        if (!vulkanCreateDeviceIfNeeded()) return false;

        //查询绘制面的能力，以确定飞行帧的数量等
        if (!vulkanGetSurfaceCapabilitiesIfNeeded()) return false;

        if (!vulkanCreateSwapchainIfNeeded()) return false;

        if (!vulkanCreateRenderPassIfNeeded()) return false;

        if (!vulkanCreateDescriptorSetLayoutIfNeeded()) return false;

        if (!vulkanCreateGraphicsPipelineIfNeeded()) return false;

        if (!vulkanCreateCommandPoolIfNeeded()) return false;

        if (!vulkanCreateDescriptorPoolIfNeeded()) return false;

        if (!vulkanCreateFrameResourcesIfNeeded()) return false;

        if (!vulkanCreateSwapchainResourcesIfNeeded()) return false;

        retro_hw_context_reset_t reset_func = coreCtx->GetRenderHWContextResetCallback();
        if (reset_func && !VK_BIT_TEST(surfaceContext_.flags, RRVULKAN_SURFACE_STATE_CORE_CONTEXT_LOADED)) {
            LOGD_VVC("Call context reset function.");
            reset_func();
        }
        VK_BIT_SET(surfaceContext_.flags, RRVULKAN_SURFACE_STATE_CORE_CONTEXT_LOADED);

        vulkanIsReady_ = true;
        return true;
    }

    void VulkanVideoContext::Unload() {
        vulkanIsReady_ = false;
        LOGD_VVC("VulkanVideoContext::Unload called, frame count: %lu", frameCount_);

        auto appContext = AppContext::Current();
        auto coreCtx = appContext->GetCoreRuntimeContext();
        auto gameCtx = appContext->GetGameRuntimeContext();

        if (logicalDevice_) {
            vkDeviceWaitIdle(logicalDevice_);
        }
        retro_hw_context_reset_t destroy_func = coreCtx->GetRenderHWContextDestroyCallback();
        if (destroy_func) destroy_func();

        vulkanClearSwapchainResourcesIfNeeded();
        vulkanClearFrameResourcesIfNeeded();
        vulkanClearSwapchainIfNeeded();
        if (surfaceContext_.surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance_, surfaceContext_.surface, nullptr);
            surfaceContext_.surface = VK_NULL_HANDLE;
            surfaceContext_.flags = RRVULKAN_SURFACE_STATE_INVALID;
        }
    }

    void VulkanVideoContext::Destroy() {
        if (queue_lock) {
            pthread_mutex_destroy(queue_lock);
            delete queue_lock;
            queue_lock = nullptr;
        }
    }

    void VulkanVideoContext::SetWindowPaused() {
        vulkanIsReady_ = false;
    }

    void VulkanVideoContext::UpdateVideoSize(unsigned int width, unsigned int height) {
        //TODO: 在这里确认是否需要重建上下文
        screen_height_ = height;
        screen_width_ = width;
    }

    void VulkanVideoContext::Prepare() {
        if (vulkanIsReady_) vulkanRecreateSwapchainIfNeeded();
    }

    void VulkanVideoContext::OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) {
        if (data) {
            vulkanCreateDrawingResourceIfNeeded(width, height);
            if (data == RETRO_HW_FRAME_BUFFER_VALID) {
                //LOGD_VVC("OnNewFrame called with RETRO_HW_FRAME_BUFFER_VALID, this is a hardware render frame.");
            } else {
                fillFrameTexture(data, width, height, pitch);
                //DRAW_LOGD_VVC("OnNewFrame called with data: %p, width: %u, height: %u, pitch: %zu, sw: %u, sh: %u", data, width, height, pitch, screen_width_, screen_height_);
            }
            if (vulkanIsReady_) {
                DrawFrame();
            }

        } else {
            LOGW_VVC("OnNewFrame called with null data, this is not a valid frame.");
        }
    }

    void VulkanVideoContext::DrawFrame() {
        if (vulkanIsReady_) {
            pthread_mutex_lock(queue_lock);
            vulkanCommitFrame();
            pthread_mutex_unlock(queue_lock);
        }
        //LOGD_VVC("DrawFrame : %lu", frameCount_);
        frameCount_++;
    }

    bool VulkanVideoContext::TakeScreenshot(const std::string &path) {
        return false;
    }

    bool VulkanVideoContext::getRetroHardwareRenderInterface(void **interface) {
        if (retro_render_interface_ == nullptr) {
            retro_render_interface_ = new retro_hw_render_interface_vulkan{
                    .interface_type = RETRO_HW_RENDER_INTERFACE_VULKAN,
                    .interface_version = RETRO_HW_RENDER_INTERFACE_VULKAN_VERSION,
                    .handle = this,
                    .instance = instance_,
                    .gpu = physicalDevice_,
                    .device = logicalDevice_,

                    .get_device_proc_addr = vkGetDeviceProcAddr,
                    .get_instance_proc_addr = vkGetInstanceProcAddr,
                    .queue = presentationQueue_,
                    .queue_index = presentationQueueFamilyIndex_,

                    .set_image = retro_vulkan_set_image_t_impl_wrapper,
                    .get_sync_index = retro_vulkan_get_sync_index_t_impl_wrapper,
                    .get_sync_index_mask = retro_vulkan_get_sync_index_mask_t_impl_wrapper,
                    .set_command_buffers = retro_vulkan_set_command_buffers_t_impl_wrapper,
                    .wait_sync_index = retro_vulkan_wait_sync_index_t_impl_wrapper,
                    .lock_queue = retro_vulkan_lock_queue_t_impl_wrapper,
                    .unlock_queue = retro_vulkan_unlock_queue_t_impl_wrapper,
                    .set_signal_semaphore = retro_vulkan_set_signal_semaphore_t_impl_wrapper

            };
        }
        *interface = retro_render_interface_;
        return true;
    }

    void VulkanVideoContext::vulkanCommitFrame() {
        if (!videoContentNeedUpdate_) return;
        auto &frame = renderContext_.frames[renderContext_.current_frame];

        vkWaitForFences(logicalDevice_, 1, &frame.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(logicalDevice_, 1, &frame.fence);

        uint32_t image_index;
        vkAcquireNextImageKHR(logicalDevice_, swapchainContext_.swapchain, UINT64_MAX, frame.imageAcquireSemaphore, VK_NULL_HANDLE, &image_index);

        recordCommandBufferForSoftwareRender(frame.commandBuffer, image_index, frame.descriptorSet);

        VkQueue queue = presentationQueue_;


        VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &frame.imageAcquireSemaphore,
                .pWaitDstStageMask = &waitStageMask,
                .commandBufferCount = 1,
                .pCommandBuffers = &frame.commandBuffer,
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &frame.renderSemaphore
        };
        VkResult submitResult = vkQueueSubmit(queue, 1, &submit_info, frame.fence);


        if (submitResult != VK_SUCCESS) {
            LOGE_VC("frame: %lu, Failed to submit queue: %d", frameCount_, submitResult);
            return;
        } else {
            //LOGI_VC("frame: %lu, Queue submitted successfully, image: %d", frameCount_, renderContext_.current_frame);
        }


        VkResult result = VK_SUCCESS;
        VkPresentInfoKHR presentInfo{
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &frame.renderSemaphore,
                .swapchainCount = 1,
                .pSwapchains = &swapchainContext_.swapchain,
                .pImageIndices = &image_index,
                .pResults = &result,
        };
        vkQueuePresentKHR(queue, &presentInfo);

        //LOGI_VVC("frame: %lu, Queue present complete, image index: %u, result: %d", frameCount_, renderContext_.current_frame, result);
        renderContext_.current_frame = (renderContext_.current_frame + 1) % renderContext_.frames.size();
        videoContentNeedUpdate_ = false;
    }

    void VulkanVideoContext::recordCommandBufferForSoftwareRender(void *pCommandBuffer, uint32_t imageIndex, VkDescriptorSet frameDescriptorSet) {
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        auto commandBuffer = (VkCommandBuffer) pCommandBuffer;
        VkCommandBufferBeginInfo beginInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        };
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        VkRenderPassBeginInfo renderPassInfo{};
        {
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderContext_.renderPass;
            renderPassInfo.framebuffer = swapchainContext_.frameBuffers[imageIndex];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = surfaceContext_.extent;

            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;
        }
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderContext_.pipeline);
        VkViewport viewport{
                .width = (float) screen_width_,
                .height = (float) screen_height_,
                .minDepth = 0.0f,
                .maxDepth = 1.0f
        };
        VkRect2D scissor{
                .offset = {0, 0},
                .extent = {screen_width_, screen_height_}
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkImageView imageViewToBePresent = VK_NULL_HANDLE;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        if(negotiationImage_ && negotiationImage_->image_view){
            imageViewToBePresent = negotiationImage_->image_view;
            imageLayout = negotiationImage_->image_layout;
        }
        if(imageViewToBePresent == VK_NULL_HANDLE){
            auto& frame = renderContext_.frames[renderContext_.current_frame];
            imageViewToBePresent = frame.texture.imageView;
            imageLayout = frame.texture.layout;
        }
        if (imageViewToBePresent) {

            //绑定顶点缓冲区
            VkDeviceSize offsets[] = {0};
            VkBuffer vertexBuffer = vertexBuffer_->getBuffer();
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

            VkDescriptorSet descriptorSet = frameDescriptorSet;

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = imageLayout;
            imageInfo.imageView = imageViewToBePresent;
            imageInfo.sampler = sampler_;


            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSet;
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfo;
            //LOGD_VVC("update descriptor %p, set: %p, image: %p", &descriptorWrite, descriptorSet, imageViewToBePresent);
            vkUpdateDescriptorSets(logicalDevice_, 1, &descriptorWrite, 0, nullptr);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderContext_.pipelineLayout,
                                    0, 1, &descriptorSet, 0, nullptr);

            vkCmdDraw(commandBuffer, 6, 1, 0, 0);

        }

        negotiationImage_ = nullptr; // reset after draw

        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);
    }

    //=== VULKAN methods ==============================

    const retro_hw_render_context_negotiation_interface_vulkan *VulkanVideoContext::getNegotiationInterface() {
        auto interface = AppContext::Current()->GetCoreRuntimeContext()->GetRenderHWNegotiationInterface();
        if (interface && interface->interface_type == RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN) {
            return (const retro_hw_render_context_negotiation_interface_vulkan *) interface;
        }
        return nullptr;
    }

    bool VulkanVideoContext::vulkanCreateInstanceIfNeeded() {
        if (instance_) return true;
        uint32_t apiVersionCheck = VK_API_VERSION_1_0;
        vkEnumerateInstanceVersion(&apiVersionCheck);
        LOGD_VVC("Current Vulkan API Version: %u.%u.%u",
                 VK_VERSION_MAJOR(apiVersionCheck),
                 VK_VERSION_MINOR(apiVersionCheck),
                 VK_VERSION_PATCH(apiVersionCheck));

        const retro_hw_render_context_negotiation_interface_vulkan *negotiator = getNegotiationInterface();
        VkApplicationInfo appInfo{
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pNext = nullptr,
                .pApplicationName = "RetroRunner",
                .applicationVersion = 0,
                .pEngineName = "RetroRunner",
                .engineVersion = 0,
                .apiVersion = VK_API_VERSION_1_0,
        };
        if (negotiator) {
            //如果上下文协商接口版本 >= 2并且提供了应用程序信息获取函数, 则需要提供应用程序信息
            if (!negotiator->get_application_info && negotiator->interface_version >= 2) {
                LOGE_VVC("Negotiation interface do not provide application info as it is required by v2 ");
                return false;
            }
            if (negotiator->get_application_info) {
                const VkApplicationInfo *extAppInfo = negotiator->get_application_info();
                if (!extAppInfo && negotiator->interface_version >= 2) {
                    LOGE_VVC("Negotiation interface provide a null application info as it is required by v2 ");
                    return false;
                }
                if (extAppInfo) {
                    appInfo = *extAppInfo;
                }
            }
        }

        if (appInfo.pApplicationName) {
            LOGD_VVC("vulkan app: %s", appInfo.pApplicationName);
        }
        if (appInfo.pEngineName) {
            LOGD_VVC("vulkan engine: %s", appInfo.pEngineName);
        }

        //如果Vulkan API版本小于1.1, 则检查是否可以使用1.1版本, 需要使用1.1的API以实现更多的功能
        if (appInfo.apiVersion < VK_API_VERSION_1_1) {
            unsigned supportedVersion;
            if (vkEnumerateInstanceVersion && (VK_SUCCESS == vkEnumerateInstanceVersion(&supportedVersion)) && (supportedVersion >= VK_API_VERSION_1_1)) {
                appInfo.apiVersion = VK_API_VERSION_1_1;
            }
        }

        LOGD_VVC("Use Vulkan API Version: %u.%u.%u",
                 VK_VERSION_MAJOR(appInfo.apiVersion),
                 VK_VERSION_MINOR(appInfo.apiVersion),
                 VK_VERSION_PATCH(appInfo.apiVersion));

        if (negotiator && negotiator->interface_version >= 2 && negotiator->create_instance) {
            instance_ = negotiator->create_instance(vkGetInstanceProcAddr, &appInfo, retro_vulkan_create_instance_wrapper_t_impl, this);
        } else {
            VkInstanceCreateInfo createInfo{
                    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .pApplicationInfo = &appInfo,
                    .enabledLayerCount = 0,
                    .ppEnabledLayerNames = nullptr,
                    .enabledExtensionCount = 0,
                    .ppEnabledExtensionNames = nullptr
            };
            instance_ = this->retro_vulkan_create_instance_wrapper(&createInfo); // retro_vulkan_create_instance_wrapper_t_impl(this, &createInfo);
        }
        if (instance_ == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to create instance.");
            return false;
        }
        return true;
    }

    bool VulkanVideoContext::vulkanCreateSurfaceIfNeeded() {
        auto appContext = AppContext::Current();
        AppWindow appWindow = appContext->GetAppWindow();
        if (surfaceContext_.surface != VK_NULL_HANDLE && surfaceContext_.surfaceId == appWindow.surfaceId) {
            return true;
        }
        surfaceContext_.flags = RRVULKAN_SURFACE_STATE_INVALID;

        window_ = appWindow.window;
        surfaceContext_.surfaceId = appWindow.surfaceId;
        screen_width_ = appWindow.width;
        screen_height_ = appWindow.height;

        surfaceContext_.extent = {
                .width = screen_width_,
                .height = screen_height_,
        };

        VkAndroidSurfaceCreateInfoKHR createInfo{
                .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .window = (ANativeWindow *) window_
        };
        VkResult createRet = vkCreateAndroidSurfaceKHR(instance_, &createInfo, nullptr, &surfaceContext_.surface);

        if (createRet != VK_SUCCESS || surfaceContext_.surface == VK_NULL_HANDLE) {
            LOGE_VC("Surface create failed:  %d\n", createRet);
            return false;
        }
        VK_BIT_SET(surfaceContext_.flags, RRVULKAN_SURFACE_STATE_VALID);
        LOGD_VC("Surface:\t\t%p\n", surfaceContext_.surface);
        return true;
    }

    bool VulkanVideoContext::vulkanChooseGPU() {
        uint32_t gpuCount = 0;
        std::vector<VkPhysicalDevice> gpus{};

        VkResult result = vkEnumeratePhysicalDevices(instance_, &gpuCount, nullptr);
        if (result != VK_SUCCESS || gpuCount == 0) {
            LOGE_VVC("failed to get physical device count.");
            return false;
        }
        gpus.resize(gpuCount);
        result = vkEnumeratePhysicalDevices(instance_, &gpuCount, gpus.data());
        if (result != VK_SUCCESS || gpuCount == 0) {
            LOGE_VVC("failed to get physical devices.");
            return false;
        }

        int graphicsFamily = -1;
        LOGI_VVC("found GPU: %d", gpuCount);
        for (auto device: gpus) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            if (graphicsFamily == -1 &&
                (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
                 deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                && isVkPhysicalDeviceSuitable(device, &graphicsFamily)) {
                physicalDevice_ = device;
                presentationQueueFamilyIndex_ = graphicsFamily;
                LOGI_VVC("\t%s , queue index: %d  -- selected", deviceProperties.deviceName, graphicsFamily);
            } else {
                LOGI_VVC("\t%s", deviceProperties.deviceName);
            }
        }
        if (graphicsFamily == -1 || physicalDevice_ == VK_NULL_HANDLE) {
            LOGE_VVC("failed to find a suitable gpu device.");
            return false;
        }
        return true;
    }

    /** TODO: Need to fix on HarmonyOS. */
    bool VulkanVideoContext::vulkanCreateDeviceIfNeeded() {
        if (logicalDevice_ != nullptr) return true;

        VkPhysicalDeviceFeatures features{false};
        const retro_hw_render_context_negotiation_interface_vulkan *negotiation = getNegotiationInterface();
        //选择合适的GPU设备
        if (!vulkanChooseGPU()) return false;

        //如果使用上下文协商接口, 则使用协商接口创建设备
        if (negotiation && negotiation->create_device) {
            retro_vulkan_context vk_context{};
            vk_context.gpu = physicalDevice_;

            bool createResult = false;
            if (negotiation->interface_version > 1 && negotiation->create_device2) {
                LOGD_VVC("create device (2) with our selected gpu.");
                createResult = negotiation->create_device2(
                        &vk_context, instance_, physicalDevice_,
                        surfaceContext_.surface, vkGetInstanceProcAddr,
                        retro_vulkan_create_device_wrapper_t_impl, this);
                if (!createResult) {
                    LOGW_VVC("Failed to create device (2) on provided GPU device, now let the core choose.");
                    createResult = negotiation->create_device2(
                            &vk_context, instance_, VK_NULL_HANDLE,
                            surfaceContext_.surface, vkGetInstanceProcAddr,
                            retro_vulkan_create_device_wrapper_t_impl, this);
                }
            } else {
                std::vector<const char *> deviceExtensions{};
                deviceExtensions.push_back("VK_KHR_swapchain");
                LOGD_VVC("create device with our selected gpu, surface: %p", surfaceContext_.surface);
                createResult = negotiation->create_device(
                        &vk_context, instance_, physicalDevice_, surfaceContext_.surface, vkGetInstanceProcAddr,
                        deviceExtensions.data(), deviceExtensions.size(),
                        nullptr, 0, &features);
            }
            if (createResult) {
                if (physicalDevice_ != nullptr && physicalDevice_ != vk_context.gpu) {
                    LOGE_VVC("Got unexpected gpu device, core choose a different one.");
                }
                destroyDeviceImpl_ = negotiation->destroy_device;

                logicalDevice_ = vk_context.device;
                physicalDevice_ = vk_context.gpu;

                if (vk_context.queue != vk_context.presentation_queue) {
                    LOGW_VVC("Graphic queue is not the same as presentation queue, this may cause issues. Currently not supported.");
                    return false;
                }

                presentationQueue_ = vk_context.presentation_queue;
                presentationQueueFamilyIndex_ = vk_context.presentation_queue_family_index;

                LOGD_VC("Logical device [negotiation]: %p", logicalDevice_);
                LOGD_VC("Physical queue [negotiation]: %p", physicalDevice_);
                LOGD_VC("Graphic queue [negotiation]: %p", vk_context.queue);
                LOGD_VC("Queue family index [negotiation]: %d", vk_context.queue_family_index);

                LOGD_VC("Presentation queue [negotiation]: %p", presentationQueue_);
                LOGD_VC("Presentation queue family index [negotiation]: %d", presentationQueueFamilyIndex_);
            } else {
                LOGE_VVC("Failed to create device with negotiation interface. Falling back to RetroRunner");
                return false;
            }
        }

        vkGetPhysicalDeviceProperties(physicalDevice_, &physicalDeviceProperties_);
        vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &physicalDeviceMemoryProperties_);

        LOGD_VVC("Use physical device: %s", physicalDeviceProperties_.deviceName);

        //如果没有使用协商接口, 则使用默认的方式创建设备
        if (logicalDevice_ == nullptr) {
            uint32_t queueCount = 0;
            bool queueFamilyIndexFound = false;
            std::vector<VkQueueFamilyProperties> queueFamilyProperties{};
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueCount, nullptr);
            if (queueCount < 1) {
                LOGE_VVC("no graphic queue found.");
                return false;
            }
            queueFamilyProperties.resize(queueCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueCount, queueFamilyProperties.data());

            VkQueueFlags requiredQueueFlag = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
            for (int idx = 0; idx < queueCount; idx++) {
                auto property = queueFamilyProperties[idx];
                VkBool32 supported = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, idx, surfaceContext_.surface, &supported);
                if (supported && ((property.queueFlags & requiredQueueFlag) == requiredQueueFlag)) {
                    presentationQueueFamilyIndex_ = idx;
                    queueFamilyIndexFound = true;
                    LOGD_VVC("Use queue family: %d", presentationQueueFamilyIndex_);
                    break;
                }
            }

            if (!queueFamilyIndexFound) {
                LOGE_VVC("No suitable graphics queue found.");
                return false;
            }
            std::vector<const char *> enabledExts{"VK_KHR_swapchain"};
            std::vector<const char *> optionalExts{"VK_KHR_sampler_mirror_clamp_to_edge"};

            //查找被支持的备选的设备扩展
            if (!vkFindDeviceExtensions(physicalDevice_, enabledExts, optionalExts)) {
                LOGE_VVC("Required device extensions not found.");
                return false;
            }

            static const float priority = 1.0f;
            VkDeviceQueueCreateInfo queueCreateInfo{};
            {
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.pNext = nullptr;
                queueCreateInfo.flags = 0;
                queueCreateInfo.queueFamilyIndex = presentationQueueFamilyIndex_;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &priority;
            }
            VkDeviceCreateInfo deviceCreateInfo{};
            {
                deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                deviceCreateInfo.pNext = nullptr;
                deviceCreateInfo.flags = 0;
                deviceCreateInfo.queueCreateInfoCount = 1;
                deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
                deviceCreateInfo.enabledLayerCount = 0;
                deviceCreateInfo.ppEnabledLayerNames = nullptr;
                deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExts.size());
                deviceCreateInfo.ppEnabledExtensionNames = enabledExts.data();
                deviceCreateInfo.pEnabledFeatures = &features;
            }
            VkResult result = vkCreateDevice(physicalDevice_, &deviceCreateInfo, nullptr, &logicalDevice_);
            if (result != VK_SUCCESS || logicalDevice_ == nullptr) {
                LOGE_VVC("Failed to create logical device: %d", result);
                return false;
            }
            LOGD_VVC("Vulkan logical device: %p", logicalDevice_);

            vkGetDeviceQueue(logicalDevice_, presentationQueueFamilyIndex_, 0, &presentationQueue_);
            LOGD_VC("graphics queue:\t%p", presentationQueue_);
        }

        return true;
    }

    bool VulkanVideoContext::vulkanGetSurfaceCapabilitiesIfNeeded() {
        if (!VK_BIT_TEST(surfaceContext_.flags, RRVULKAN_SURFACE_STATE_VALID)) return false;
        if (VK_BIT_TEST(surfaceContext_.flags, RRVULKAN_SURFACE_STATE_CAPABILITY_SET)) return true;

        vkDeviceWaitIdle(logicalDevice_);

        //Get surface properties
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkResult surfaceCapabilityRet = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surfaceContext_.surface, &surfaceCapabilities);
        if (surfaceCapabilityRet != VK_SUCCESS) {
            LOGE_VC("Can't get capability from surface.");
            return false;
        }

        //get pixel format
        uint32_t surfaceFormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surfaceContext_.surface, &surfaceFormatCount, nullptr);
        if (surfaceFormatCount < 1) {
            LOGE_VC("No surface format found.");
            return false;
        }
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surfaceContext_.surface, &surfaceFormatCount, surfaceFormats.data())) {
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

        framesInFlight_ = surfaceCapabilities.minImageCount;
        if (framesInFlight_ < 2) {
            LOGW_VVC("Surface minImageCount is less than 2, this may cause issues.");
            framesInFlight_ = 2; // 飞行帧的数量至少为2
        }
        if (surfaceCapabilities.maxImageCount >= framesInFlight_ + 1) {
            surfaceContext_.imageCount = framesInFlight_ + 1;
        } else {
            surfaceContext_.imageCount = framesInFlight_;
        }
        surfaceContext_.format = surfaceFormats[chosenIndex].format;
        surfaceContext_.colorSpace = surfaceFormats[chosenIndex].colorSpace;
        surfaceContext_.extent = {screen_width_, screen_height_};

        LOGD_VC("Surface chosen: [ %d x %d ], format: %d, color space: %d\n", surfaceContext_.extent.width, surfaceContext_.extent.height, surfaceContext_.format, surfaceContext_.colorSpace);

        VK_BIT_SET(surfaceContext_.flags, RRVULKAN_SURFACE_STATE_CAPABILITY_SET);
        return true;
    }

    bool VulkanVideoContext::vulkanCreateCommandPoolIfNeeded() {
        if (renderContext_.commandPool) return true;

        VkCommandPoolCreateInfo cmdPoolCreateInfo{};
        {
            cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmdPoolCreateInfo.pNext = nullptr;
            cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            cmdPoolCreateInfo.queueFamilyIndex = presentationQueueFamilyIndex_;
        };
        VkResult createCmdPoolResult = vkCreateCommandPool(logicalDevice_, &cmdPoolCreateInfo, nullptr, &renderContext_.commandPool);

        if (createCmdPoolResult != VK_SUCCESS || renderContext_.commandPool == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create command pool! %d\n", createCmdPoolResult);
            return false;
        } else {
            LOGD_VC("Command pool created: %p\n", renderContext_.commandPool);
        }
        return true;
    }

    bool VulkanVideoContext::vulkanCreateSwapchainIfNeeded() {
        if (swapchainContext_.flag & RRVULKAN_SWAPCHAIN_STATE_SWAPCHAIN_VALID) {
            LOGD_VVC("swapchain already created, skip.");
            return true;
        }
        vkDeviceWaitIdle(logicalDevice_);
        VkSwapchainCreateInfoKHR swapchainCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = nullptr,
                .surface = surfaceContext_.surface,
                .minImageCount = surfaceContext_.imageCount,

                .imageFormat = surfaceContext_.format,
                .imageColorSpace = swapchainContext_.colorSpace,
                .imageExtent = surfaceContext_.extent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,                                             // shared mode?
                .pQueueFamilyIndices = nullptr,
                .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,                   //android: VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,  windows: VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
                .presentMode = VK_PRESENT_MODE_FIFO_KHR,                                //MAILBOX, FIFO? i think we need to check which to be use.
                .clipped = VK_TRUE,
                .oldSwapchain = swapchainContext_.swapchain,                             //TODO: check maybe we need to pass the old swapchain here.
        };
        VkResult createSwapChainResult = vkCreateSwapchainKHR(logicalDevice_, &swapchainCreateInfo, nullptr, &swapchainContext_.swapchain);
        if (createSwapChainResult || swapchainContext_.swapchain == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create swap chain! %d\n", createSwapChainResult);
            return false;
        } else {
            LOGD_VC("swapchain:\t\t%p\n", swapchainContext_.swapchain);
        }
        VK_BIT_SET(swapchainContext_.flag, RRVULKAN_SWAPCHAIN_STATE_SWAPCHAIN_VALID);
        return true;
    }

    bool VulkanVideoContext::vulkanCreateRenderPassIfNeeded() {
        if (renderContext_.flag & RRVULKAN_RENDER_STATE_RENDER_PASS_VALID) return true;

        VkAttachmentDescription attachmentDescriptions{};
        {
            attachmentDescriptions.format = surfaceContext_.format;
            attachmentDescriptions.samples = VK_SAMPLE_COUNT_1_BIT;
            attachmentDescriptions.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescriptions.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescriptions.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescriptions.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescriptions.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescriptions.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        VkAttachmentReference colourReference = {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpassDescription{};
        {
            subpassDescription.flags = 0;
            subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription.inputAttachmentCount = 0;
            subpassDescription.pInputAttachments = nullptr;
            subpassDescription.colorAttachmentCount = 1;
            subpassDescription.pColorAttachments = &colourReference;
            subpassDescription.pResolveAttachments = nullptr;
            subpassDescription.pDepthStencilAttachment = nullptr;
            subpassDescription.preserveAttachmentCount = 0;
            subpassDescription.pPreserveAttachments = nullptr;
        }
        VkRenderPassCreateInfo renderPassCreateInfo{};
        {
            renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassCreateInfo.pNext = nullptr;
            renderPassCreateInfo.attachmentCount = 1;
            renderPassCreateInfo.pAttachments = &attachmentDescriptions;
            renderPassCreateInfo.subpassCount = 1;
            renderPassCreateInfo.pSubpasses = &subpassDescription;
            renderPassCreateInfo.dependencyCount = 0;
            renderPassCreateInfo.pDependencies = nullptr;
        }
        VkResult createRenderPassResult = vkCreateRenderPass(logicalDevice_, &renderPassCreateInfo, nullptr, &renderContext_.renderPass);
        if (createRenderPassResult || renderContext_.renderPass == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create render pass: %d\n", createRenderPassResult);
            return false;
        } else {
            LOGD_VC("Render pass created: %p\n", renderContext_.renderPass);
            VK_BIT_SET(renderContext_.flag, RRVULKAN_RENDER_STATE_RENDER_PASS_VALID);
        }
        return true;
    }

    bool VulkanVideoContext::vulkanCreateDescriptorSetLayoutIfNeeded() {
        if (VK_BIT_TEST(renderContext_.flag, RRVULKAN_RENDER_STATE_DESCRIPTOR_SET_LAYOUT_VALID)) return true;

        //检测是否需要创建Shader
        if (!VK_BIT_TEST(renderContext_.flag, RRVULKAN_RENDER_STATE_SHADERS_VALID)) {
            //这里应该使用动态加载的方式来加载Shader代码
            if (!createShader((void *) shader_vertex_code, shader_vertex_code_size, VulkanShaderType::SHADER_VERTEX, &renderContext_.vertexShaderModule)) {
                LOGE_VC("failed to create vertex shader.");
                return false;
            }
            if (!createShader((void *) shader_fragment_code, shader_fragment_code_size, VulkanShaderType::SHADER_FRAGMENT, &renderContext_.fragmentShaderModule)) {
                LOGE_VC("failed to create fragment shader.");
                return false;
            }
            VK_BIT_SET(renderContext_.flag, RRVULKAN_RENDER_STATE_SHADERS_VALID);
        }

        //纹理输入描述 fragment binding(pipeline layout) should base on fragment shader
        VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{};
        {
            descriptorSetLayoutBinding.binding = 0;
            descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorSetLayoutBinding.descriptorCount = 1;
            descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        {
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.pNext = nullptr;
            descriptorSetLayoutCreateInfo.bindingCount = 1;
            descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;
        };

        VkResult descSetLayoutCreateResult = vkCreateDescriptorSetLayout(logicalDevice_, &descriptorSetLayoutCreateInfo, nullptr, &renderContext_.descriptorSetLayout);
        if (descSetLayoutCreateResult != VK_SUCCESS || renderContext_.descriptorSetLayout == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create descriptor set layout: %d", descSetLayoutCreateResult);
            return false;
        }
        LOGD_VC("Descriptor set layout created: %p", renderContext_.descriptorSetLayout);

        renderContext_.flag |= RRVULKAN_RENDER_STATE_DESCRIPTOR_SET_LAYOUT_VALID;

        return true;

    }

    bool VulkanVideoContext::vulkanCreateGraphicsPipelineIfNeeded() {
        if (!(renderContext_.flag & RRVULKAN_RENDER_STATE_RENDER_PASS_VALID)) return false;
        if (VK_BIT_TEST(renderContext_.flag, RRVULKAN_RENDER_STATE_PIPELINE_VALID)) return true;
        //管线布局
        if (!VK_BIT_TEST(renderContext_.flag, RRVULKAN_RENDER_STATE_PIPELINE_LAYOUT_VALID)) {
            VkPipelineLayoutCreateInfo layoutCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                    .setLayoutCount = 1,
                    .pSetLayouts =  renderContext_.descriptorSetLayout ? &renderContext_.descriptorSetLayout : nullptr
            };
            VkResult layoutCreateResult = vkCreatePipelineLayout(logicalDevice_, &layoutCreateInfo, nullptr, &renderContext_.pipelineLayout);
            if (layoutCreateResult != VK_SUCCESS || renderContext_.pipelineLayout == VK_NULL_HANDLE) {
                LOGE_VC("Failed to create pipeline layout: %d", layoutCreateResult);
                return false;
            }
            VK_BIT_SET(renderContext_.flag, RRVULKAN_RENDER_STATE_PIPELINE_LAYOUT_VALID);
        }

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

        pipelineCreateInfo.renderPass = renderContext_.renderPass;
        pipelineCreateInfo.layout = renderContext_.pipelineLayout;

        // 动态调整视图大小与裁剪区域
        VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.dynamicStateCount = 2;
        dynamicStateInfo.pDynamicStates = dynamicStates;

        pipelineCreateInfo.pDynamicState = &dynamicStateInfo;

        VkViewport viewport{};
        {
            viewport.width = (float) surfaceContext_.extent.width;
            viewport.height = (float) surfaceContext_.extent.width;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
        }
        VkRect2D scissor{};
        {
            scissor.offset = {0, 0};
            scissor.extent = {surfaceContext_.extent.width, surfaceContext_.extent.height};
        }
        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
        {
            viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportStateCreateInfo.viewportCount = 1;
            viewportStateCreateInfo.pViewports = &viewport;
            viewportStateCreateInfo.scissorCount = 1;
            viewportStateCreateInfo.pScissors = &scissor;
        }
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;

        //vertex assembly 图元组装
        VkPipelineInputAssemblyStateCreateInfo assemblyStateCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,                           //图元的组装方式为 TOPOLOGY_TRIANGLE_LIST, 每相邻3个点组成一个三角形
                .primitiveRestartEnable = VK_FALSE
        };
        pipelineCreateInfo.pInputAssemblyState = &assemblyStateCreateInfo;


        // 顶点数据绑定描述, 每6个float为一组
        VkVertexInputBindingDescription vertexInputBindingDescription{};
        {
            vertexInputBindingDescription.binding = 0;
            vertexInputBindingDescription.stride = 6 * sizeof(float);                                            //size of point
            vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        }
        //一组顶点数据包含2个属性，顶点位置(x,y,z,w), 纹理位置(x,y)
        VkVertexInputAttributeDescription attributeDescriptions[2]{};
        {
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;                                                          //对应顶点着色器的location 0
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[0].offset = 0;

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;                                                          //对应顶点着色器的location 1
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = 4 * sizeof(float);
        }

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
        {
            vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
            vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
            vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2;
            vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions;
        }

        pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;


        //Shader 着色器描述
        VkPipelineShaderStageCreateInfo stageCreateInfo[2]{
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_VERTEX_BIT,
                        .module = renderContext_.vertexShaderModule,
                        .pName = "main"
                },
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                        .module = renderContext_.fragmentShaderModule,
                        .pName = "main"
                }
        };
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = stageCreateInfo;

        //rasterizer 光栅化设置
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .pNext = nullptr,
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode = VK_POLYGON_MODE_FILL,        //填充模式
                .cullMode = VK_CULL_MODE_NONE,              //根据面向淘汰的方式
                .frontFace = VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable = VK_FALSE,
                .lineWidth = 1,
        };
        pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;

        //多重采样 multi-sample
        VkSampleMask sampleMask = ~0u;
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                .pSampleMask = &sampleMask,
        };
        pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;

        //depth, stencil 深度与模板测试

        //color blending 颜色混合模式
        VkPipelineColorBlendAttachmentState colorBlendAttachmentState{
                .blendEnable = VK_FALSE,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };
        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_COPY,
                .attachmentCount = 1,
                .pAttachments = &colorBlendAttachmentState,
        };

        pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;

        //管线缓存
        if (!VK_BIT_TEST(renderContext_.flag, RRVULKAN_RENDER_STATE_PIPELINE_CACHE_VALID)) {
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
                    .flags = 0
            };
            VkResult pipelineCacheCreateResult = vkCreatePipelineCache(logicalDevice_, &pipelineCacheCreateInfo, nullptr, &renderContext_.pipelineCache);
            if (pipelineCacheCreateResult != VK_SUCCESS || renderContext_.pipelineCache == VK_NULL_HANDLE) {
                LOGE_VC("pipeline cache create failed: %d", pipelineCacheCreateResult);
                return false;
            } else {
                LOGD_VC("pipeline cache:\t%p", renderContext_.pipelineCache);
            }
            VK_BIT_SET(renderContext_.flag, RRVULKAN_RENDER_STATE_PIPELINE_CACHE_VALID);
        }

        VkResult pipelineCreateResult = vkCreateGraphicsPipelines(logicalDevice_, renderContext_.pipelineCache, 1, &pipelineCreateInfo, nullptr, &renderContext_.pipeline);
        if (pipelineCreateResult != VK_SUCCESS || renderContext_.pipeline == VK_NULL_HANDLE) {
            LOGE_VC("pipeline create failed: %d\n", pipelineCreateResult);
            return false;
        } else {
            LOGD_VC("pipeline:\t\t%p", renderContext_.pipeline);
        }
        VK_BIT_SET(renderContext_.flag, RRVULKAN_RENDER_STATE_PIPELINE_VALID);
        return true;
    }

    bool VulkanVideoContext::vulkanCreateDescriptorPoolIfNeeded() {
        if (renderContext_.flag & RRVULKAN_RENDER_STATE_DESCRIPTOR_POOL_VALID) return true;
        VkDescriptorPoolSize poolSize{
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,              //绑定纹理
                .descriptorCount = 16                                           //每帧会有一个绑定
        };
        VkDescriptorPoolCreateInfo poolCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,     //允许释放描述符集
                .maxSets = 16,                                                  //每帧会有一个绑定,数量需要大于飞行帧数量
                .poolSizeCount = 1,
                .pPoolSizes = &poolSize
        };
        //VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
        VkResult poolCreateResult = vkCreateDescriptorPool(logicalDevice_, &poolCreateInfo, nullptr, &renderContext_.descriptorPool);
        if (poolCreateResult != VK_SUCCESS || renderContext_.descriptorPool == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create descriptor pool: %d", poolCreateResult);
            return false;
        }
        LOGD_VC("Descriptor pool created: %p", renderContext_.descriptorPool);
        renderContext_.flag |= RRVULKAN_RENDER_STATE_DESCRIPTOR_POOL_VALID;
        return true;
    }

    bool VulkanVideoContext::vulkanCreateFrameResourcesIfNeeded() {
        renderContext_.frames.resize(framesInFlight_);
        bool resourceValid = true;

        for (uint32_t i = 0; i < framesInFlight_; i++) {
            RRVulkanFrameContext &frame = renderContext_.frames[i];
            // 分配描述符集（使用全局的 DescriptorSetLayout）
            VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            {
                allocInfo.descriptorPool = renderContext_.descriptorPool;            // 全局描述符池
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = &renderContext_.descriptorSetLayout;         // 共享的布局
            };
            VkResult createDescriptorSetsResult = vkAllocateDescriptorSets(logicalDevice_, &allocInfo, &frame.descriptorSet);
            if (createDescriptorSetsResult != VK_SUCCESS || frame.descriptorSet == VK_NULL_HANDLE) {
                LOGE_VC("Failed to allocate descriptor set for frame %d, %d", i, createDescriptorSetsResult);
                resourceValid = false;
                break;
            }

            //分配CommandBuffer
            VkCommandBufferAllocateInfo cmdAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            {
                cmdAllocInfo.pNext = nullptr;
                cmdAllocInfo.commandPool = renderContext_.commandPool; // 使用全局的 CommandPool
                cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                cmdAllocInfo.commandBufferCount = 1;
            }
            VkResult cmdAllocResult = vkAllocateCommandBuffers(logicalDevice_, &cmdAllocInfo, &frame.commandBuffer);
            if (cmdAllocResult != VK_SUCCESS || frame.commandBuffer == VK_NULL_HANDLE) {
                LOGE_VC("Failed to allocate command buffer for frame %d: %d", i, cmdAllocResult);
                resourceValid = false;
                break;
            }

            VkFenceCreateInfo fenceCreateInfo{};
            {
                fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceCreateInfo.pNext = nullptr;
                fenceCreateInfo.flags = 0;
            }

            VkResult createFenceResult = vkCreateFence(logicalDevice_, &fenceCreateInfo, nullptr, &frame.fence);
            if (createFenceResult != VK_SUCCESS || frame.fence == VK_NULL_HANDLE) {
                LOGE_VC("Failed to create fence! %d\n", createFenceResult);
                resourceValid = false;
                break;
            }
            vkResetFences(logicalDevice_, 1, &frame.fence);

            VkSemaphoreCreateInfo imageAcquireSemaphoreCreateInfo{};
            {
                imageAcquireSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                imageAcquireSemaphoreCreateInfo.pNext = nullptr;
                imageAcquireSemaphoreCreateInfo.flags = 0;
            }
            VkResult imageAcquireCreateSemaphoreResult = vkCreateSemaphore(logicalDevice_, &imageAcquireSemaphoreCreateInfo, nullptr, &frame.imageAcquireSemaphore);
            if (imageAcquireCreateSemaphoreResult != VK_SUCCESS || frame.imageAcquireSemaphore == VK_NULL_HANDLE) {
                LOGE_VC("Failed to create image acquire semaphore! %d", imageAcquireCreateSemaphoreResult);
                resourceValid = false;
                break;
            }

            VkSemaphoreCreateInfo renderSemaphoreCreateInfo{};
            {
                renderSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                renderSemaphoreCreateInfo.pNext = nullptr;
                renderSemaphoreCreateInfo.flags = 0;
            }
            VkResult renderCreateSemaphoreResult = vkCreateSemaphore(logicalDevice_, &renderSemaphoreCreateInfo, nullptr, &frame.renderSemaphore);
            if (renderCreateSemaphoreResult != VK_SUCCESS || frame.renderSemaphore == VK_NULL_HANDLE) {
                LOGE_VC("Failed to create render semaphore! %d", renderCreateSemaphoreResult);
                resourceValid = false;
                break;
            }
        }

        if (!resourceValid) {
            LOGE_VC("Failed to create frame resources, will not continue.");
            vulkanClearFrameResourcesIfNeeded();
            return false;
        }

        return true;
    }

    bool VulkanVideoContext::vulkanCreateSwapchainResourcesIfNeeded() {
        bool resourceValid = true;

        if (!VK_BIT_TEST(swapchainContext_.flag, RRVULKAN_SWAPCHAIN_STATE_IMAGES_VALID)) {
            do {

                uint32_t imageCount = 0;
                VkResult getSwapchainImagesResult = vkGetSwapchainImagesKHR(logicalDevice_, swapchainContext_.swapchain, &imageCount, nullptr);

                LOGD_VVC("swapchain image count: %d, calced count: %d", imageCount, surfaceContext_.imageCount);

                swapchainContext_.images.resize(surfaceContext_.imageCount);
                vkGetSwapchainImagesKHR(logicalDevice_, swapchainContext_.swapchain, &surfaceContext_.imageCount, swapchainContext_.images.data());

                //image views
                swapchainContext_.imageViews.resize(surfaceContext_.imageCount);
                for (uint32_t imgIdx = 0; imgIdx < surfaceContext_.imageCount; imgIdx++) {
                    VkImageViewCreateInfo viewCreateInfo = {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                            .pNext = nullptr,
                            .flags = 0,
                            .image = swapchainContext_.images[imgIdx],
                            .viewType = VK_IMAGE_VIEW_TYPE_2D,
                            .format = surfaceContext_.format,
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
                    if (vkCreateImageView(logicalDevice_, &viewCreateInfo, nullptr, &swapchainContext_.imageViews[imgIdx]) != VK_SUCCESS || swapchainContext_.imageViews[imgIdx] == VK_NULL_HANDLE) {
                        LOGE_VC("Failed to create swapchain image view [%d]!\n", imgIdx);
                        resourceValid = false;
                        break;
                    }
                }
                LOGD_VC("Swapchain image views created: %d\n", surfaceContext_.imageCount);

                if (!resourceValid) break;

                //frame buffers
                swapchainContext_.frameBuffers.resize(surfaceContext_.imageCount);
                for (int idx = 0; idx < surfaceContext_.imageCount; idx++) {
                    VkImageView attachmentViews[] = {swapchainContext_.imageViews[idx]};
                    VkFramebufferCreateInfo fbCreateInfo{
                            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                            .pNext = nullptr,
                            .renderPass = renderContext_.renderPass,
                            .attachmentCount = 1,                                   // (depthView == VK_NULL_HANDLE ? 1 : 2),
                            .pAttachments = attachmentViews,
                            .width = static_cast<uint32_t>(surfaceContext_.extent.width),
                            .height = static_cast<uint32_t>(surfaceContext_.extent.height),
                            .layers = 1,
                    };
                    if (vkCreateFramebuffer(logicalDevice_, &fbCreateInfo, nullptr, &swapchainContext_.frameBuffers[idx]) != VK_SUCCESS || swapchainContext_.frameBuffers[idx] == VK_NULL_HANDLE) {
                        LOGE_VC("Failed to create frame buffer %d!\n", idx);
                        resourceValid = false;
                        break;
                    }
                }
                LOGD_VC("Swapchain frame buffers created: %d\n", surfaceContext_.imageCount);

            } while (false);

            if (resourceValid) {
                VK_BIT_SET(swapchainContext_.flag, RRVULKAN_SWAPCHAIN_STATE_IMAGES_VALID);
            }
        }

        if (!resourceValid) {
            LOGE_VC("Failed to create fences for swapchain images, will not continue.");
            vulkanClearSwapchainResourcesIfNeeded();
            return false;
        }
        return true;
    }

    void VulkanVideoContext::vulkanCreateDrawingResourceIfNeeded(uint32_t width, uint32_t height) {
        if (!vulkanIsReady_) return;
        if (width > 0 && height > 0) {
            auto &frame = renderContext_.frames[renderContext_.current_frame];
            if (frame.texture.imageView == VK_NULL_HANDLE || frame.texture.width != width || frame.texture.height != height) {
                LOGD_VVC("create frame texture if needed: %d x %d", width, height);
                if (frame.texture.imageView != VK_NULL_HANDLE) {
                    vkDestroyImageView(logicalDevice_, frame.texture.imageView, nullptr);
                    frame.texture.imageView = VK_NULL_HANDLE;
                }
                if (frame.texture.image != VK_NULL_HANDLE) {
                    vkDestroyImage(logicalDevice_, frame.texture.image, nullptr);
                    frame.texture.image = VK_NULL_HANDLE;
                }
                if (frame.texture.memory != VK_NULL_HANDLE) {
                    vkFreeMemory(logicalDevice_, frame.texture.memory, nullptr);
                    frame.texture.memory = VK_NULL_HANDLE;
                }

                frame.texture.width = 0;
                frame.texture.height = 0;
                do {
                    VkImageCreateInfo imageCreateInfo{};
                    {
                        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
                        imageCreateInfo.format = surfaceContext_.format;
                        imageCreateInfo.extent.width = width;
                        imageCreateInfo.extent.height = height;
                        imageCreateInfo.extent.depth = 1;
                        imageCreateInfo.mipLevels = 1;
                        imageCreateInfo.arrayLayers = 1;
                        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    };
                    VkResult imageCreateResult = vkCreateImage(logicalDevice_, &imageCreateInfo, nullptr, &frame.texture.image);
                    if (imageCreateResult != VK_SUCCESS || frame.texture.image == VK_NULL_HANDLE) {
                        LOGE_VC("Failed to create texture image: %d", imageCreateResult);
                        break;
                    }
                    frame.texture.layout = imageCreateInfo.initialLayout;

                    LOGD_VC("Texture image created: %p", frame.texture.image);
                    VkMemoryRequirements memoryRequirements{};
                    vkGetImageMemoryRequirements(logicalDevice_, frame.texture.image, &memoryRequirements);
                    VkMemoryAllocateInfo memoryAllocateInfo{};
                    {
                        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                        memoryAllocateInfo.allocationSize = memoryRequirements.size;
                        memoryAllocateInfo.memoryTypeIndex = VkUtil::findMemoryType(physicalDevice_, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);;
                    }
                    VkResult memoryAllocateResult = vkAllocateMemory(logicalDevice_, &memoryAllocateInfo, nullptr, &frame.texture.memory);
                    if (memoryAllocateResult != VK_SUCCESS || frame.texture.memory == VK_NULL_HANDLE) {
                        LOGE_VC("Failed to allocate texture memory: %d", memoryAllocateResult);
                        break;
                    }
                    LOGD_VC("Texture memory allocated: %p", frame.texture.memory);
                    VkResult bindImageMemoryResult = vkBindImageMemory(logicalDevice_, frame.texture.image, frame.texture.memory, 0);
                    if (bindImageMemoryResult != VK_SUCCESS) {
                        LOGE_VC("Failed to bind texture image memory: %d", bindImageMemoryResult);
                        break;
                    }
                    LOGD_VC("Texture image memory bound: %p", frame.texture.memory);
                    VkImageViewCreateInfo imageViewCreateInfo{};
                    {
                        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                        imageViewCreateInfo.image = frame.texture.image;
                        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                        imageViewCreateInfo.format = surfaceContext_.format;
                        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
                        imageViewCreateInfo.subresourceRange.levelCount = 1;
                        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
                        imageViewCreateInfo.subresourceRange.layerCount = 1;
                    }
                    VkResult imageViewCreateResult = vkCreateImageView(logicalDevice_, &imageViewCreateInfo, nullptr, &frame.texture.imageView);
                    if (imageViewCreateResult != VK_SUCCESS || frame.texture.imageView == VK_NULL_HANDLE) {
                        LOGE_VC("Failed to create texture image view: %d", imageViewCreateResult);
                        break;
                    }
                    LOGD_VC("Texture image view created: %p", frame.texture.imageView);


                    frame.texture.width = width;
                    frame.texture.height = height;
                } while (false);
            }
        }
        if (vertexBuffer_ == nullptr) {
            vertexBuffer_ = new VulkanRWBuffer(physicalDevice_, logicalDevice_, presentationQueueFamilyIndex_);
            //如果核心没有提供渲染接口，那么软件渲染一般使用的是opengl生成的图片, 图片则需要使用opengl的坐标
            if(retro_render_interface_ == nullptr){
                vertexBuffer_->create(sizeof(verticesOpenGL), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
                vertexBuffer_->update(verticesOpenGL, sizeof(verticesOpenGL));
            }else {
                vertexBuffer_->create(sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
                vertexBuffer_->update(vertices, sizeof(vertices));
            }
        }
        if (sampler_ == VK_NULL_HANDLE) {
            VkFilter filter = VK_FILTER_NEAREST;
            VkSamplerCreateInfo samplerInfo{};
            {
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = filter;
                samplerInfo.minFilter = filter;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

                samplerInfo.anisotropyEnable = VK_FALSE;  //各向异性
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
            }
        }
    }

    void VulkanVideoContext::vulkanRecreateSwapchainIfNeeded() {
        if (!VK_BIT_TEST(swapchainContext_.flag, RRVULKAN_SWAPCHAIN_STATE_NEED_RECREATE)) {
            return;
        }
        vulkanIsReady_ = false;
        LOGD_VVC("Recreating swapchain...");

        vkDeviceWaitIdle(logicalDevice_);

        vulkanClearSwapchainResourcesIfNeeded();
        vulkanClearSwapchainIfNeeded();

        if (!vulkanCreateSwapchainIfNeeded()) {
            LOGE_VC("Failed to create swapchain, will not continue.");
            return;
        }
        if (!vulkanCreateSwapchainResourcesIfNeeded()) {
            LOGE_VC("Failed to create swapchain resources, will not continue.");
            return;
        }
        vulkanIsReady_ = true;
        VK_BIT_CLEAR(swapchainContext_.flag, RRVULKAN_SWAPCHAIN_STATE_NEED_RECREATE);
    }

    bool VulkanVideoContext::createShader(void *source, size_t sourceLength, VulkanShaderType shaderType, VkShaderModule *shader) {
        VkShaderModuleCreateInfo createInfo{
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = sourceLength,
                .pCode = (const uint32_t *) source,
        };
        VkResult result = vkCreateShaderModule(logicalDevice_, &createInfo, nullptr, shader);
        if (result != VK_SUCCESS || *shader == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create shader module! %d", result);
            return false;
        } else {
            LOGD_VC("Shader module created: %p", *shader);
            return true;
        }
    }

    void VulkanVideoContext::copyNegotiationImageToFrameTexture() {
        if (!negotiationImage_) return;

    }

    void VulkanVideoContext::fillFrameTexture(const void *data, unsigned int width, unsigned int height, size_t pitch) {
        if (!vulkanIsReady_) return;
        auto frame = &renderContext_.frames[renderContext_.current_frame];

        if (frame->stagingBuffer.buffer == VK_NULL_HANDLE) {
            VkBufferCreateInfo createBufferInfo{
                    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .size = width * height * 4,
                    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                    .queueFamilyIndexCount = 1,
                    .pQueueFamilyIndices = &presentationQueueFamilyIndex_
            };
            VkResult result = vkCreateBuffer(logicalDevice_, &createBufferInfo, nullptr, (VkBuffer *) &frame->stagingBuffer.buffer);
            if (result != VK_SUCCESS) {
                LOGE_VC("failed to create staging buffer, error %d", result);
                return;
            }
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(logicalDevice_, (VkBuffer) frame->stagingBuffer.buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            VkUtil::MapMemoryTypeToIndex(
                    physicalDevice_,
                    memRequirements.memoryTypeBits,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &allocInfo.memoryTypeIndex);
            result = vkAllocateMemory(logicalDevice_, &allocInfo, nullptr, (VkDeviceMemory *) &frame->stagingBuffer.memory);
            if (result != VK_SUCCESS) {
                LOGE_VC("failed to allocate staging buffer memory, error %d", result);
            }
            frame->stagingBuffer.size = allocInfo.allocationSize;
            vkBindBufferMemory(logicalDevice_, (VkBuffer) frame->stagingBuffer.buffer, (VkDeviceMemory) frame->stagingBuffer.memory, 0);
        }

        //使用data填充frame的纹理
        const void *finalData = data;
        if (core_pixel_format_ == RETRO_PIXEL_FORMAT_RGB565) {
            convertingBuffer.resize(width * height * 4);
            VkUtil::convertRGB565ToRGBA8888(data, width, height, convertingBuffer.data());
            finalData = convertingBuffer.data();
        } else if (core_pixel_format_ == RETRO_PIXEL_FORMAT_0RGB1555) {
            //TODO: 检测pixel格式转换 RETRO_PIXEL_FORMAT_0RGB1555
            LOGE_VVC("0RGB1555 pixel format is not supported yet, please use RGB565 or RGBA8888.");
        }
        //copy data to staging buffer
        void *toData;
        vkMapMemory(logicalDevice_, frame->stagingBuffer.memory, 0, frame->stagingBuffer.size, 0, &toData);
        //TODO: 这里需要检测pitch, 拷贝数据的长度，数据超出范围或者拷贝不全
        //size_t  copySize = min(frame->stagingBuffer.size, pitch * height);
        memcpy(toData, finalData,frame->stagingBuffer.size );
        vkUnmapMemory(logicalDevice_, frame->stagingBuffer.memory);

        // 转换图像布局为 `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`
        VkUtil::transitionImageLayout(logicalDevice_, renderContext_.commandPool, presentationQueue_, frame->texture.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &frame->texture.layout);


        //copy staging buffer to texture image
        VkCommandBuffer commandBuffer = VkUtil::beginSingleTimeCommands(logicalDevice_, renderContext_.commandPool);
        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {width, height, 1};
        vkCmdCopyBufferToImage(commandBuffer, frame->stagingBuffer.buffer, frame->texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        VkUtil::endSingleTimeCommands(logicalDevice_, renderContext_.commandPool, presentationQueue_, commandBuffer);

        //转换图像布局回 `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`
        VkUtil::transitionImageLayout(logicalDevice_, renderContext_.commandPool, presentationQueue_, frame->texture.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &frame->texture.layout);

        //LOGD_VVC("Frame texture updated: %d x %d, core pixel format: %d", width, height, core_pixel_format_);
        videoContentNeedUpdate_ = true;
    }

}

namespace libRetroRunner {

    bool VulkanVideoContext::vulkanClearSwapchainResourcesIfNeeded() {

        for (auto &frameBuffer: swapchainContext_.frameBuffers) {
            if (frameBuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(logicalDevice_, frameBuffer, nullptr);
                frameBuffer = VK_NULL_HANDLE;
            }
        }
        swapchainContext_.frameBuffers.clear();
        for (auto &imageView: swapchainContext_.imageViews) {
            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(logicalDevice_, imageView, nullptr);
                imageView = VK_NULL_HANDLE;
            }
        }
        swapchainContext_.imageViews.clear();
        VK_BIT_CLEAR(swapchainContext_.flag, RRVULKAN_SWAPCHAIN_STATE_IMAGES_VALID);

        return true;
    }

    bool VulkanVideoContext::vulkanClearFrameResourcesIfNeeded() {
        for (auto &frame: renderContext_.frames) {
            if (frame.texture.imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(logicalDevice_, frame.texture.imageView, nullptr);
                frame.texture.imageView = VK_NULL_HANDLE;
            }
            if (frame.texture.image != VK_NULL_HANDLE) {
                vkDestroyImage(logicalDevice_, frame.texture.image, nullptr);
                frame.texture.image = VK_NULL_HANDLE;
            }
            if (frame.texture.memory != VK_NULL_HANDLE) {
                vkFreeMemory(logicalDevice_, frame.texture.memory, nullptr);
                frame.texture.memory = VK_NULL_HANDLE;
            }
            frame.texture.width = 0;
            frame.texture.height = 0;

            if (frame.fence != VK_NULL_HANDLE) {
                vkDestroyFence(logicalDevice_, frame.fence, nullptr);
                frame.fence = VK_NULL_HANDLE;
            }
            if (frame.imageAcquireSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(logicalDevice_, frame.imageAcquireSemaphore, nullptr);
                frame.imageAcquireSemaphore = VK_NULL_HANDLE;
            }
            if (frame.renderSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(logicalDevice_, frame.renderSemaphore, nullptr);
                frame.renderSemaphore = VK_NULL_HANDLE;
            }
            if (frame.descriptorSet != VK_NULL_HANDLE) {
                vkFreeDescriptorSets(logicalDevice_, renderContext_.descriptorPool, 1, &frame.descriptorSet);
                frame.descriptorSet = VK_NULL_HANDLE;
            }
            if (frame.commandBuffer != VK_NULL_HANDLE) {
                vkFreeCommandBuffers(logicalDevice_, renderContext_.commandPool, 1, &frame.commandBuffer);
                frame.commandBuffer = VK_NULL_HANDLE;
            }
        }

        renderContext_.frames.clear();
        return true;
    }

    bool VulkanVideoContext::vulkanClearSwapchainIfNeeded() {
        if (swapchainContext_.swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(logicalDevice_, swapchainContext_.swapchain, nullptr);
            swapchainContext_.swapchain = VK_NULL_HANDLE;
        }
        swapchainContext_.flag = RRVULKAN_SWAPCHAIN_STATE_INVALID;
        return true;
    }

    void VulkanVideoContext::clearVulkanRenderContext() {
        delete[] renderContext_.fragmentShaderCode;
        delete[] renderContext_.vertexShaderCode;
        if (renderContext_.vertexShaderModule) vkDestroyShaderModule(logicalDevice_, renderContext_.vertexShaderModule, nullptr);
        if (renderContext_.fragmentShaderModule) vkDestroyShaderModule(logicalDevice_, renderContext_.fragmentShaderModule, nullptr);
        if (renderContext_.pipeline) vkDestroyPipeline(logicalDevice_, renderContext_.pipeline, nullptr);
        if (renderContext_.pipelineCache) vkDestroyPipelineCache(logicalDevice_, renderContext_.pipelineCache, nullptr);

        if (renderContext_.descriptorPool) vkDestroyDescriptorPool(logicalDevice_, renderContext_.descriptorPool, nullptr);
        if (renderContext_.descriptorSetLayout) vkDestroyDescriptorSetLayout(logicalDevice_, renderContext_.descriptorSetLayout, nullptr);
        if (renderContext_.pipelineLayout) vkDestroyPipelineLayout(logicalDevice_, renderContext_.pipelineLayout, nullptr);
        if (renderContext_.renderPass) vkDestroyRenderPass(logicalDevice_, renderContext_.renderPass, nullptr);
        memset(&renderContext_, 0, sizeof(RRVulkanRenderContext));
        LOGD_VC("pipeline cleared");
    }

    void VulkanVideoContext::clearDrawingResourceIfNeeded() {

    }
}

namespace libRetroRunner {
    bool isVkPhysicalDeviceSuitable(VkPhysicalDevice device, int *graphicsFamily) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
        *graphicsFamily = -1;

        for (int idx = 0; idx < queueFamilies.size(); idx++) {
            const auto &queueFamily = queueFamilies[idx];
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                *graphicsFamily = idx;
                return true;
            }
        }
        return false;
    }

    bool vkFindExtension(const char *ext, std::vector<VkExtensionProperties> &properties) {
        for (auto property: properties) {
            const char *propertyName = property.extensionName;
            if (ext && propertyName && !strcmp(ext, propertyName)) {
                return true;
            }
        }
        return false;
    }

    bool vkFindExtensions(const std::vector<const char *> &exts, std::vector<VkExtensionProperties> &properties) {
        for (auto ext: exts) {
            if (!vkFindExtension(ext, properties)) {
                LOGE_VVC("extension [ %s ] is not supported.", ext);
                return false;
            }
        }
        return true;
    }

    bool vkFindInstanceExtensions(const std::vector<const char *> &exts) {
        uint32_t propertyCount = 0;
        std::vector<VkExtensionProperties> properties{};
        bool ret = true;
        if (vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr) != VK_SUCCESS)
            return false;
        do {
            if (propertyCount == 0) break;
            properties.resize(propertyCount);
            if (vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, properties.data()) != VK_SUCCESS)
                break;
            ret = vkFindExtensions(exts, properties);
        } while ((false));

        return ret;
    }

    bool vkFindDeviceExtensions(VkPhysicalDevice gpu, std::vector<const char *> &enabledExts, std::vector<const char *> &optionalExts) {
        uint32_t propertyCount = 0;
        if (vkEnumerateDeviceExtensionProperties(gpu, nullptr, &propertyCount, nullptr) != VK_SUCCESS || propertyCount == 0) {
            LOGE_VVC("Failed to get device extension property for %p", gpu);
            return false;
        }
        std::vector<VkExtensionProperties> properties{};
        properties.resize(propertyCount);

        if (vkEnumerateDeviceExtensionProperties(gpu, nullptr, &propertyCount, properties.data()) != VK_SUCCESS || propertyCount == 0) {
            LOGE_VVC("Failed to get device extension properties, 0/%d.", propertyCount);
            return false;
        }

        if (!vkFindExtensions(enabledExts, properties)) {
            return false;
        }

        for (auto optionalExt: optionalExts) {
            if (vkFindExtension(optionalExt, properties)) {
                enabledExts.push_back(optionalExt);
                LOGD_VVC("Enable optional device extension: %s", optionalExt);
            }
        }

        return true;
    }

    VkInstance VulkanVideoContext::retro_vulkan_create_instance_wrapper(const VkInstanceCreateInfo *create_info) const {
        VkInstanceCreateInfo info = *create_info;
        VkInstance instance = VK_NULL_HANDLE;
        std::vector<const char *> layers{};
        std::vector<const char *> extensions{};
        extensions.push_back("VK_KHR_surface");
        extensions.push_back("VK_KHR_android_surface");

        //如果需要调试层, 需要把对应的库放到libs目录下 (libVkLayer_khronos_validation.so)
        if (is_vulkan_debug_) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
        }

        uint32_t layerCount = 128;
        VkLayerProperties layerProperties[128];
        vkEnumerateInstanceLayerProperties(&layerCount, layerProperties);

        if (layerCount == 0 && !vkFindInstanceExtensions(extensions)) {
            LOGE_VVC("Instance does not support required extensions.");
            return VK_NULL_HANDLE;
        }

        //check vulkan api version
        if (info.pApplicationInfo) {
            uint32_t supportedVersion = VK_API_VERSION_1_0;
            if (!vkEnumerateInstanceVersion || vkEnumerateInstanceVersion(&supportedVersion) != VK_SUCCESS)
                supportedVersion = VK_API_VERSION_1_0;
            if (supportedVersion < info.pApplicationInfo->apiVersion) {
                LOGE_VVC("core required vulkan with api: %u.u, but this device do not support.",
                         VK_VERSION_MAJOR(info.pApplicationInfo->apiVersion),
                         VK_VERSION_MINOR(info.pApplicationInfo->apiVersion));
                return VK_NULL_HANDLE;
            }
        }

        info.enabledLayerCount = static_cast<uint32_t>(layers.size());
        info.ppEnabledLayerNames = layers.data();

        info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();

        VkResult result = vkCreateInstance(&info, NULL, &instance);
        if (result != VK_SUCCESS || !instance) {
            LOGE_VVC("Failed to create vulkan instance: %d, if vkCreateInstance is called with debug layers, make sure vulkan support these layers.", result);
            return VK_NULL_HANDLE;
        } else {
            LOGD_VVC("Vulkan instance: %p", instance);
        }
        return instance;
    }

    VkDevice VulkanVideoContext::retro_vulkan_create_device_wrapper(VkPhysicalDevice gpu, const VkDeviceCreateInfo *create_info) {
        LOGD_VVC("execute retro_vulkan_create_device_wrapper");
        VkDeviceCreateInfo info = *create_info;
        VkDevice device = VK_NULL_HANDLE;

        std::vector<const char *> extensions{};
        std::vector<const char *> optionalExts{"VK_KHR_sampler_mirror_clamp_to_edge"};

        for (int idx = 0; idx < info.enabledExtensionCount; idx++) {
            extensions.push_back(info.ppEnabledExtensionNames[idx]);
        }
        extensions.push_back("VK_KHR_swapchain");

        if (!vkFindDeviceExtensions(gpu, extensions, optionalExts)) {
            LOGE_VVC("Required device extensions not found.");
            return VK_NULL_HANDLE;
        }

        info.ppEnabledExtensionNames = extensions.data();
        info.enabledExtensionCount = extensions.size();
        VkResult result = vkCreateDevice(gpu, &info, nullptr, &device);
        if (result != VK_SUCCESS || device == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to create logical device: %d", result);
        } else {
            LOGD_VVC("Vulkan logical device: %p", device);
        }
        return device;
    }
}
