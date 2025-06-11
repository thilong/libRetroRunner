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

#define CHECK_VK_OBJ_NOT_NULL(arg, msg) if (arg == nullptr) { LOGE_VVC(msg); return false; }

void *getVKApiAddress(const char *sym) {
    void *libvulkan = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!libvulkan)
        return 0;
    return dlsym(libvulkan, sym);
}

/** misc functions */
namespace libRetroRunner {
    bool isVkPhysicalDeviceSuitable(VkPhysicalDevice device, int *graphicsFamily);

    bool vkFindDeviceExtensions(VkPhysicalDevice gpu, std::vector<const char *> &enabledExts, std::vector<const char *> &optionalExts);
}

/** negotiation interface and hardware rendering interface implementation */
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

    void VulkanVideoContext::retro_vulkan_set_image_t_impl(const struct retro_vulkan_image *image, uint32_t num_semaphores, const VkSemaphore *semaphores, uint32_t src_queue_family) {
        negotiationImage_ = image;
       if(!negotiationSemaphores_){
           negotiationSemaphores_ = new VkSemaphore[20];
       }
       if(!negotiationWaitStages_){
              negotiationWaitStages_ = new VkPipelineStageFlags[20];
       }
       negotiationSemaphoreCount_ = num_semaphores;
       if(semaphores){
            for (uint32_t i = 0; i < num_semaphores; ++i) {
                if(i > 19){
                    LOGE_VVC("semaphores count %u is too large, max is 20", num_semaphores);
                    break;
                }
                negotiationSemaphores_[i] = semaphores[i];
                negotiationWaitStages_[i] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
        }
        negotiationQueueFamily_ = src_queue_family;


       //TODO: check line:3080 vk->flags |= VK_FLAG_HW_VALID_SEMAPHORE;
        if(image){
            LOGW_VVC("need to implement retro_vulkan_set_image_t_impl %p, %d, %d, %u, %p, %u", image->image_view, image->image_layout, image->create_info.flags, num_semaphores, semaphores, src_queue_family);
        }else{
            LOGW_VVC("need to implement retro_vulkan_set_image_t_impl %p, %u, %p, %u", image, num_semaphores, semaphores, src_queue_family);
        }
    }

    uint32_t VulkanVideoContext::retro_vulkan_get_sync_index_t_impl() {
        uint32_t ret = swapchainContext_.valid ? swapchainContext_.current_image : 0;
        //LOGW_VVC("call retro_vulkan_get_sync_index_t_impl with return: %u", ret);
        return 0;
    }

    uint32_t VulkanVideoContext::retro_vulkan_get_sync_index_mask_t_impl() {
        uint32_t ret = swapchainContext_.valid ?  ((1 << swapchainContext_.imageCount) - 1) : 0;
        LOGW_VVC("call retro_vulkan_get_sync_index_mask_t_impl with return: %u", ret);
        return ret;
    }

    void VulkanVideoContext::retro_vulkan_set_command_buffers_t_impl(uint32_t num_cmd, const VkCommandBuffer *cmd) {
        LOGW_VVC("need to implement retro_vulkan_set_command_buffers_t_impl");
    }

    void VulkanVideoContext::retro_vulkan_wait_sync_index_t_impl() {
        //LOGW_VVC("need to implement retro_vulkan_wait_sync_index_t_impl");
    }

    void VulkanVideoContext::retro_vulkan_lock_queue_t_impl() {
        //LOGW_VVC("need to implement retro_vulkan_lock_queue_t_impl");
        /*
        #ifdef HAVE_THREADS
                vk_t *vk = (vk_t*)handle;
           slock_lock(vk->context->queue_lock);
        #endif
         */
    }

    void VulkanVideoContext::retro_vulkan_unlock_queue_t_impl() {
        //LOGW_VVC("need to implement retro_vulkan_unlock_queue_t_impl");
        /*
        #ifdef HAVE_THREADS
                vk_t *vk = (vk_t*)handle;
           slock_unlock(vk->context->queue_lock);
        #endif
        */
    }

    void VulkanVideoContext::retro_vulkan_set_signal_semaphore_t_impl(VkSemaphore semaphore) {
        //TODO: LOGW_VVC("need to implement retro_vulkan_set_signal_semaphore_t_impl");
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
        is_new_surface_ = true;
        screen_width_ = 1;
        screen_height_ = 100;
        is_ready_ = false;
        retro_vk_context_ = nullptr;
        retro_render_interface_ = nullptr;
        core_pixel_format_ = RETRO_PIXEL_FORMAT_XRGB8888;

        window_ = nullptr;

        instance_ = VK_NULL_HANDLE;
        physicalDevice_ = VK_NULL_HANDLE;
        queueFamilyIndex_ = 0;
        logicalDevice_ = VK_NULL_HANDLE;
        graphicQueue_ = VK_NULL_HANDLE;

        commandPool_ = VK_NULL_HANDLE;

        destroyDeviceImpl_ = nullptr;
        negotiationSemaphores_ = nullptr;
        negotiationWaitStages_ = nullptr;
    }

    VulkanVideoContext::~VulkanVideoContext() {
        Destroy();
    }

    bool VulkanVideoContext::Load() {
        auto appContext = AppContext::Current();
        auto coreCtx = appContext->GetCoreRuntimeContext();
        auto gameCtx = appContext->GetGameRuntimeContext();
        core_pixel_format_ = coreCtx->GetPixelFormat();
        //step:load vulkan library
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

        //step: check negotiation interface
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


        //step: create instance if it is null.
        if (!vulkanCreateInstanceIfNeeded()) return false;

        //step: create surface
        if (!vulkanCreateSurfaceIfNeeded()) return false;

        //step: create devices
        if (!vulkanCreateDeviceIfNeeded()) return false;

        //step: command pool
        if (!vulkanCreateCommandPoolIfNeeded()) return false;

        //step: pipeline
        if (!vulkanCreateCommandPoolIfNeeded()) return false;

        if (!vulkanCreateRenderContextIfNeeded()) return false;

        //step: create swapchain L:1684
        if (!vulkanCreateSwapchainIfNeeded()) return false;

        //step: call context_reset
        retro_hw_context_reset_t reset_func = coreCtx->GetRenderHWContextResetCallback();
        if(is_new_surface_) {
            if (reset_func) {
                LOGD_VVC("Call context reset function.");
                reset_func();
            }
            is_new_surface_ = false;
        }

        vulkanIsReady_ = true;
        return true;
    }

    void VulkanVideoContext::Unload() {
        //context_destroy
        vulkanIsReady_ = false;
    }

    void VulkanVideoContext::Destroy() {
        if (retro_vk_context_) {
            delete retro_vk_context_;
            retro_vk_context_ = nullptr;
        }
    }

    void VulkanVideoContext::UpdateVideoSize(unsigned int width, unsigned int height) {
        screen_height_ = height;
        screen_width_ = width;
    }

    void VulkanVideoContext::Prepare() {
        if(vulkanIsReady_){

        }
    }

    void VulkanVideoContext::OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) {

        if(data){
            if(data == RETRO_HW_FRAME_BUFFER_VALID) {
                //LOGD_VVC("OnNewFrame called with RETRO_HW_FRAME_BUFFER_VALID, this is a hardware render frame.");
            } else {
                //LOGD_VVC("OnNewFrame called with data: %p, width: %u, height: %u, pitch: %zu", data, width, height, pitch);
            }
            if(vulkanIsReady_)
                DrawFrame();
        }else{
            LOGW_VVC("OnNewFrame called with null data, this is not a valid frame.");
        }
    }

    void VulkanVideoContext::DrawFrame() {

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
                    .queue = graphicQueue_,
                    .queue_index = queueFamilyIndex_,

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

        if (appInfo.apiVersion < VK_API_VERSION_1_1) {
            unsigned supportedVersion;
            if (vkEnumerateInstanceVersion && (VK_SUCCESS == vkEnumerateInstanceVersion(&supportedVersion)) && (supportedVersion >= VK_API_VERSION_1_1)) {
                appInfo.apiVersion = VK_API_VERSION_1_1;
            }
        }

        //TODO: check cache vk context?

        if (negotiator && negotiator->interface_version >= 2 && negotiator->create_instance) {
            instance_ = negotiator->create_instance(vkGetInstanceProcAddr, &appInfo, retro_vulkan_create_instance_wrapper_t_impl, this);
        } else {
            VkInstanceCreateInfo createInfo{
                    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                    .pNext = nullptr, .flags = 0,
                    .pApplicationInfo = &appInfo,
                    .enabledLayerCount = 0, .ppEnabledLayerNames = nullptr,
                    .enabledExtensionCount = 0,
                    .ppEnabledExtensionNames = nullptr
            };
            instance_ = retro_vulkan_create_instance_wrapper_t_impl(this, &createInfo);
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
        if (swapchainContext_.surface != VK_NULL_HANDLE && swapchainContext_.surfaceId == appWindow.surfaceId) {
            is_new_surface_ = false;
            return true;
        }
        is_new_surface_ = true;

        window_ = appWindow.window;
        swapchainContext_.surfaceId = appWindow.surfaceId;

        VkAndroidSurfaceCreateInfoKHR createInfo{
                .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .window = (ANativeWindow *) window_
        };
        VkResult createRet = vkCreateAndroidSurfaceKHR(instance_, &createInfo, nullptr, &swapchainContext_.surface);

        if (createRet != VK_SUCCESS || swapchainContext_.surface == VK_NULL_HANDLE) {
            LOGE_VC("Surface create failed:  %d\n", createRet);
            return false;
        }
        LOGD_VC("Surface:\t\t%p\n", swapchainContext_.surface);

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
                queueFamilyIndex_ = graphicsFamily;
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

    /* TODO: Need to fix.
     * this function will always return false and lead to crash on HarmonyOS,
     * because the vulkan implementation in HarmonyOS is different from Android.
     * */
    bool VulkanVideoContext::vulkanCreateDeviceIfNeeded() {
        if (logicalDevice_ != nullptr) return true;

        VkPhysicalDeviceFeatures features{false};
        const retro_hw_render_context_negotiation_interface_vulkan *negotiation = getNegotiationInterface();
        if (!vulkanChooseGPU()) return false;
        if (negotiation && negotiation->create_device) {
            struct retro_vulkan_context context = {0};
            bool createResult = false;
            if (negotiation->interface_version > 1 && negotiation->create_device2) {
                LOGD_VVC("create device (2) with our selected gpu.");
                createResult = negotiation->create_device2(
                        &context, instance_, physicalDevice_,
                        swapchainContext_.surface, vkGetInstanceProcAddr,
                        retro_vulkan_create_device_wrapper_t_impl, this);
                if (!createResult) {
                    LOGW_VVC("Failed to create device (2) on provided GPU device, now let the core choose.");
                    createResult = negotiation->create_device2(
                            &context, instance_, VK_NULL_HANDLE,
                            swapchainContext_.surface, vkGetInstanceProcAddr,
                            retro_vulkan_create_device_wrapper_t_impl, this);
                }
            } else {
                std::vector<const char *> deviceExtensions{};
                deviceExtensions.push_back("VK_KHR_swapchain");
                LOGD_VVC("create device with our selected gpu.");
                createResult = negotiation->create_device(
                        &context, instance_, physicalDevice_, swapchainContext_.surface, vkGetInstanceProcAddr,
                        deviceExtensions.data(), deviceExtensions.size(),
                        nullptr, 0, &features);
            }
            if (createResult) {
                if (physicalDevice_ != nullptr && physicalDevice_ != context.gpu) {
                    LOGE_VVC("Got unexpected gpu device, core choose a different one.");
                }
                destroyDeviceImpl_ = negotiation->destroy_device;

                logicalDevice_ = context.device;
                graphicQueue_ = context.queue;
                physicalDevice_ = context.gpu;
                queueFamilyIndex_ = context.queue_family_index;
            } else {
                LOGE_VVC("Failed to create device with negotiation interface. Falling back to RetroRunner");
                return false;
            }
        }

        vkGetPhysicalDeviceProperties(physicalDevice_, &physicalDeviceProperties_);
        vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &physicalDeviceMemoryProperties_);

        LOGD_VVC("Use physical device: %s", physicalDeviceProperties_.deviceName);

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
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, idx, swapchainContext_.surface, &supported);
                if (supported && ((property.queueFlags & requiredQueueFlag) == requiredQueueFlag)) {
                    queueFamilyIndex_ = idx;
                    queueFamilyIndexFound = true;
                    LOGD_VVC("Use queue family: %d", queueFamilyIndex_);
                    break;
                }
            }

            if (!queueFamilyIndexFound) {
                LOGE_VVC("No suitable graphics queue found.");
                return false;
            }
            std::vector<const char *> enabledExts{"VK_KHR_swapchain"};
            std::vector<const char *> optionalExts{"VK_KHR_sampler_mirror_clamp_to_edge"};
            if (!vkFindDeviceExtensions(physicalDevice_, enabledExts, optionalExts)) {
                LOGE_VVC("Required device extensions not found.");
                return false;
            }

            static const float priority = 1.0f;
            VkDeviceQueueCreateInfo queueCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = nullptr, .flags = 0,
                    .queueFamilyIndex = queueFamilyIndex_,
                    .queueCount = 1,
                    .pQueuePriorities = &priority
            };
            VkDeviceCreateInfo deviceCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                    .pNext = nullptr, .flags = 0,
                    .queueCreateInfoCount = 1,
                    .pQueueCreateInfos = &queueCreateInfo,
                    .enabledLayerCount = 0, .ppEnabledLayerNames = nullptr,
                    .enabledExtensionCount = static_cast<uint32_t>(enabledExts.size()),
                    .ppEnabledExtensionNames = enabledExts.data(),
                    .pEnabledFeatures = &features,
            };
            VkResult result = vkCreateDevice(physicalDevice_, &deviceCreateInfo, nullptr, &logicalDevice_);
            if (result != VK_SUCCESS || logicalDevice_ == nullptr) {
                LOGE_VVC("Failed to create logical device: %d", result);
                return false;
            }
            LOGD_VVC("Vulkan logical device: %p", logicalDevice_);
        }

        return true;
    }

    bool VulkanVideoContext::vulkanCreateCommandPoolIfNeeded() {
        if (commandPool_) return true;
        VkCommandPoolCreateInfo cmdPoolCreateInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = queueFamilyIndex_,
        };
        VkResult createCmdPoolResult = vkCreateCommandPool(logicalDevice_, &cmdPoolCreateInfo, nullptr, &commandPool_);

        if (createCmdPoolResult != VK_SUCCESS || commandPool_ == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create command pool! %d\n", createCmdPoolResult);
            return false;
        } else {
            LOGD_VC("Command pool created: %p\n", commandPool_);
        }
        return true;
    }

    bool VulkanVideoContext::vulkanCreateSwapchainIfNeeded() {
        if (swapchainContext_.valid) return true;
        LOGD_VVC("VulkanVideoContext::vulkanCreateSwapchainIfNeeded is called.");

        vkDeviceWaitIdle(logicalDevice_);
        //surface is new, means swapchain should be rebuild.
        if(is_new_surface_) {
            LOGD_VC("Surface is new, clear swapchain if needed.");
            vulkanClearSwapchainIfNeeded();
            //todo: remove swapchain image fences
        }

        //Get swapchain properties
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkResult surfaceCapabilityRet = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, swapchainContext_.surface, &surfaceCapabilities);
        if (surfaceCapabilityRet != VK_SUCCESS) {
            LOGE_VC("Can't get capability from surface.");
            return false;
        }
        uint32_t surfaceFormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, swapchainContext_.surface, &surfaceFormatCount, nullptr);
        if (surfaceFormatCount < 1) {
            LOGE_VC("No surface format found.");
            return false;
        }
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        if (VK_SUCCESS != vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, swapchainContext_.surface, &surfaceFormatCount, surfaceFormats.data())) {
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
        swapchainContext_.extent = surfaceCapabilities.currentExtent;
        swapchainContext_.format = surfaceFormats[chosenIndex].format;
        swapchainContext_.colorSpace = surfaceFormats[chosenIndex].colorSpace;
        swapchainContext_.minImageCount = surfaceCapabilities.minImageCount;
        LOGD_VC("Surface chosen: [ %d x %d ], format: %d, color space: %d\n", swapchainContext_.extent.width, swapchainContext_.extent.height, swapchainContext_.format, swapchainContext_.colorSpace);

        VkSwapchainCreateInfoKHR swapchainCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = nullptr,
                .surface = swapchainContext_.surface,
                .minImageCount = swapchainContext_.minImageCount,
                .imageFormat = swapchainContext_.format,
                .imageColorSpace = swapchainContext_.colorSpace,
                .imageExtent = swapchainContext_.extent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &queueFamilyIndex_,
                .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, //android: VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,  windows: VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
                .presentMode = VK_PRESENT_MODE_FIFO_KHR,
                .clipped = VK_TRUE,
                .oldSwapchain = VK_NULL_HANDLE, //TODO: 注意这里，是否要在下一次创建的时候释放掉之前的swapchain
        };
        VkResult createSwapChainResult = vkCreateSwapchainKHR(logicalDevice_, &swapchainCreateInfo, nullptr, &swapchainContext_.swapchain);
        if (createSwapChainResult || swapchainContext_.swapchain == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create swap chain! %d\n", createSwapChainResult);
            return false;
        } else {
            LOGD_VC("swapchain:\t\t%p\n", swapchainContext_.swapchain);
        }

        vkGetSwapchainImagesKHR(logicalDevice_, swapchainContext_.swapchain, &swapchainContext_.imageCount, nullptr);
        LOGD_VC("Swapchain image count: %d\n", swapchainContext_.imageCount);
        swapchainContext_.images.resize(swapchainContext_.imageCount);
        vkGetSwapchainImagesKHR(logicalDevice_, swapchainContext_.swapchain, &swapchainContext_.imageCount, swapchainContext_.images.data());
        //create fences
        swapchainContext_.imageFences.resize(swapchainContext_.imageCount);
        for (uint32_t i = 0; i < swapchainContext_.imageCount; i++) {
            VkFenceCreateInfo fenceCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0
            };
            VkResult createFenceResult = vkCreateFence(logicalDevice_, &fenceCreateInfo, nullptr, &swapchainContext_.imageFences[i]);
            if (createFenceResult != VK_SUCCESS || swapchainContext_.imageFences[i] == VK_NULL_HANDLE) {
                LOGE_VC("Failed to create fence for swapchain image %d: %d\n", i, createFenceResult);
            }
        }

        swapchainContext_.valid = true;
        return true;
    }

    bool VulkanVideoContext::vulkanClearSwapchainIfNeeded() {
        swapchainContext_.valid = false;

        for(int idx=0;idx < swapchainContext_.imageFences.size(); idx++){
            if (swapchainContext_.imageFences[idx] != VK_NULL_HANDLE) {
                vkDestroyFence(logicalDevice_, swapchainContext_.imageFences[idx], nullptr);
                swapchainContext_.imageFences[idx] = VK_NULL_HANDLE;
            }
        }
        swapchainContext_.images.clear();
        swapchainContext_.imageCount = 0;
        if (swapchainContext_.swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(logicalDevice_, swapchainContext_.swapchain, nullptr);
            swapchainContext_.swapchain = VK_NULL_HANDLE;
        }

        return true;
    }

    bool VulkanVideoContext::vulkanCreateRenderContextIfNeeded() {
        if (renderContext_.valid) return true;
        //TODO: which format to use?
        renderContext_.format = VK_FORMAT_R8G8B8A8_UNORM;
        do {
            //render pass
            VkAttachmentDescription attachmentDescriptions{
                    .format = renderContext_.format,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            };
            VkAttachmentReference colourReference = {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            VkSubpassDescription subpassDescription{
                    .flags = 0,
                    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                    .inputAttachmentCount = 0,
                    .pInputAttachments = nullptr,
                    .colorAttachmentCount = 1,
                    .pColorAttachments = &colourReference,
                    .pResolveAttachments = nullptr,
                    .pDepthStencilAttachment = nullptr,
                    .preserveAttachmentCount = 0,
                    .pPreserveAttachments = nullptr,
            };
            VkRenderPassCreateInfo renderPassCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                    .pNext = nullptr,
                    .attachmentCount = 1,
                    .pAttachments = &attachmentDescriptions,
                    .subpassCount = 1,
                    .pSubpasses = &subpassDescription,
                    .dependencyCount = 0,
                    .pDependencies = nullptr,
            };
            VkResult createRenderPassResult = vkCreateRenderPass(logicalDevice_, &renderPassCreateInfo, nullptr, &renderContext_.renderPass);

            if (createRenderPassResult || renderContext_.renderPass == VK_NULL_HANDLE) {
                LOGE_VC("Failed to create render pass: %d\n", createRenderPassResult);
                break;
            } else {
                LOGD_VC("Render pass created: %p\n", renderContext_.renderPass);
            }
            // create pipeline

            // dynamic state, let viewport and scissor dynamic, can change at runtime
            VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
            dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicStateInfo.dynamicStateCount = 2;
            dynamicStateInfo.pDynamicStates = dynamicStates;

            VkGraphicsPipelineCreateInfo pipelineCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                    .renderPass = renderContext_.renderPass
            };
            pipelineCreateInfo.pDynamicState = &dynamicStateInfo;

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

            VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                    .viewportCount = 1,
                    .pViewports = &viewport,
                    .scissorCount = 1,
                    .pScissors = &scissor
            };
            pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;

            //vertex assembly
            VkPipelineInputAssemblyStateCreateInfo assemblyStateCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,                       //TOPOLOGY_TRIANGLE_LIST, 3 points for a triangle
                    .primitiveRestartEnable = VK_FALSE
            };
            pipelineCreateInfo.pInputAssemblyState = &assemblyStateCreateInfo;

            //TODO: create shaders
            if (!createShader((void *) shader_vertex_code, shader_vertex_code_size, VulkanShaderType::SHADER_VERTEX, &renderContext_.vertexShaderModule)) {
                LOGE_VC("failed to create vertex shader.");
            }
            if (!createShader((void *) shader_fragment_code, shader_fragment_code_size, VulkanShaderType::SHADER_FRAGMENT, &renderContext_.fragmentShaderModule)) {
                LOGE_VC("failed to create fragment shader.");
            }


            //vertex binding, should base on vertex shader
            VkVertexInputBindingDescription vertexInputBindingDescription{
                    .binding = 0,
                    .stride = 6 * sizeof(float),                                            //size of point
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            };
            VkVertexInputAttributeDescription attributeDescriptions[2]{};
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[0].offset = 0;

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = 4 * sizeof(float);

            VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                    .vertexBindingDescriptionCount = 1,
                    .pVertexBindingDescriptions = &vertexInputBindingDescription,
                    .vertexAttributeDescriptionCount = 2,
                    .pVertexAttributeDescriptions = attributeDescriptions
            };
            pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;

            //fragment binding(pipeline layout) should base on fragment shader
            const VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr,
            };

            const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                    .pNext = nullptr,
                    .bindingCount = 1,
                    .pBindings = &descriptorSetLayoutBinding,
            };

            VkResult descSetLayoutCreateResult = vkCreateDescriptorSetLayout(logicalDevice_, &descriptorSetLayoutCreateInfo, nullptr, &renderContext_.descriptorSetLayout);
            if (descSetLayoutCreateResult != VK_SUCCESS || renderContext_.descriptorSetLayout == VK_NULL_HANDLE) {
                LOGE_VC("Failed to create descriptor set layout: %d", descSetLayoutCreateResult);
                break;
            }
            LOGD_VC("Descriptor set layout created: %p", renderContext_.descriptorSetLayout);

            VkDescriptorPoolSize poolSize{
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,   //绑定纹理
                    .descriptorCount = 1                                 //最多1个绑定
            };
            VkDescriptorPoolCreateInfo poolCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                    .pNext = nullptr,
                    .maxSets = 1,
                    .poolSizeCount = 1,
                    .pPoolSizes = &poolSize
            };
            VkResult poolCreateResult = vkCreateDescriptorPool(logicalDevice_, &poolCreateInfo, nullptr, &renderContext_.descriptorPool);
            if (poolCreateResult != VK_SUCCESS || renderContext_.descriptorPool == VK_NULL_HANDLE) {
                LOGE_VC("Failed to create descriptor pool: %d", poolCreateResult);
                break;
            }
            LOGD_VC("Descriptor pool created: %p", renderContext_.descriptorPool);

            VkDescriptorSetAllocateInfo allocateInfo{
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .descriptorPool =  renderContext_.descriptorPool,
                    .descriptorSetCount = 1,
                    .pSetLayouts = &renderContext_.descriptorSetLayout
            };
            VkResult allocateResult = vkAllocateDescriptorSets(logicalDevice_, &allocateInfo, &renderContext_.descriptorSet);
            if (allocateResult != VK_SUCCESS) {
                LOGE_VC("Failed to allocate descriptor set: %d", allocateResult);
                break;
            }
            LOGD_VC("Descriptor set allocated: %p", renderContext_.descriptorSet);

            //layout
            VkPipelineLayoutCreateInfo layoutCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                    .setLayoutCount = 1,
                    .pSetLayouts = renderContext_.descriptorSetLayout ? &renderContext_.descriptorSetLayout : nullptr
            };
            VkResult layoutCreateResult = vkCreatePipelineLayout(logicalDevice_, &layoutCreateInfo, nullptr, &renderContext_.pipelineLayout);
            if (layoutCreateResult != VK_SUCCESS || renderContext_.pipelineLayout == VK_NULL_HANDLE) {
                LOGE_VC("Failed to create pipeline layout: %d", layoutCreateResult);
                break;
            }
            pipelineCreateInfo.layout = renderContext_.pipelineLayout;

            //shader stage
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

            //multisample 多重采样
            VkSampleMask sampleMask = ~0u;
            VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                    .pSampleMask = &sampleMask,
            };
            pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;

            //depth, stencil 深度与模板测试

            //color blending 混合模式
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

            //cache
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
                    .flags = 0
            };
            VkResult pipelineCacheCreateResult = vkCreatePipelineCache(logicalDevice_, &pipelineCacheCreateInfo, nullptr, &renderContext_.pipelineCache);
            if (pipelineCacheCreateResult != VK_SUCCESS || renderContext_.pipelineCache == VK_NULL_HANDLE) {
                LOGE_VC("pipeline cache create failed: %d", pipelineCacheCreateResult);
                break;
            } else {
                LOGD_VC("pipeline cache:\t%p", renderContext_.pipelineCache);
            }

            VkResult pipelineCreateResult = vkCreateGraphicsPipelines(logicalDevice_, renderContext_.pipelineCache, 1, &pipelineCreateInfo, nullptr, &renderContext_.pipeline);
            if (pipelineCreateResult != VK_SUCCESS || renderContext_.pipeline == VK_NULL_HANDLE) {
                LOGE_VC("pipeline create failed: %d\n", pipelineCreateResult);
                break;
            } else {
                LOGD_VC("pipeline:\t\t%p", renderContext_.pipeline);
            }
            renderContext_.valid = true;
        } while ((false));
        if (!renderContext_.valid) {
            clearVulkanRenderContext();
        }
        return renderContext_.valid;
    }

    void VulkanVideoContext::clearVulkanRenderContext() {
        delete[] renderContext_.fragmentShaderCode;
        delete[] renderContext_.vertexShaderCode;
        if (renderContext_.vertexShaderModule) vkDestroyShaderModule(logicalDevice_, renderContext_.vertexShaderModule, nullptr);
        if (renderContext_.fragmentShaderModule) vkDestroyShaderModule(logicalDevice_, renderContext_.fragmentShaderModule, nullptr);
        if (renderContext_.pipeline) vkDestroyPipeline(logicalDevice_, renderContext_.pipeline, nullptr);
        if (renderContext_.pipelineCache) vkDestroyPipelineCache(logicalDevice_, renderContext_.pipelineCache, nullptr);
        if (!renderContext_.descriptorSet) vkFreeDescriptorSets(logicalDevice_, renderContext_.descriptorPool, 1, &renderContext_.descriptorSet);
        if (renderContext_.descriptorPool) vkDestroyDescriptorPool(logicalDevice_, renderContext_.descriptorPool, nullptr);
        if (renderContext_.descriptorSetLayout) vkDestroyDescriptorSetLayout(logicalDevice_, renderContext_.descriptorSetLayout, nullptr);
        if (renderContext_.pipelineLayout) vkDestroyPipelineLayout(logicalDevice_, renderContext_.pipelineLayout, nullptr);
        if (renderContext_.renderPass) vkDestroyRenderPass(logicalDevice_, renderContext_.renderPass, nullptr);
        memset(&renderContext_, 0, sizeof(RRVulkanRenderContext));
        renderContext_.valid = false;
        LOGD_VC("pipeline cleared");
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

    VkInstance VulkanVideoContext::retro_vulkan_create_instance_wrapper(const VkInstanceCreateInfo *create_info) {
        VkInstanceCreateInfo info = *create_info;
        VkInstance instance = VK_NULL_HANDLE;
        std::vector<const char *> layers{};
        std::vector<const char *> extensions{};
        extensions.push_back("VK_KHR_surface");
        extensions.push_back("VK_KHR_android_surface");

        if (false) {
            extensions.push_back("VK_EXT_debug_utils");
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
