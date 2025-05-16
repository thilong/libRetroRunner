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

/** retro hardware render interface implementation */
namespace libRetroRunner {

    void retro_vulkan_set_image_t_impl(void *handle,
                                       const struct retro_vulkan_image *image,
                                       uint32_t num_semaphores,
                                       const VkSemaphore *semaphores,
                                       uint32_t src_queue_family) {

    }

    uint32_t retro_vulkan_get_sync_index_t_impl(void *handle) {
        return 0;
    }

    uint32_t retro_vulkan_get_sync_index_mask_t_impl(void *handle) {
        return 0;
    }

    void retro_vulkan_set_command_buffers_t_impl(void *handle,
                                                 uint32_t num_cmd,
                                                 const VkCommandBuffer *cmd) {

    }

    void retro_vulkan_wait_sync_index_t_impl(void *handle) {

    }

    void retro_vulkan_lock_queue_t_impl(void *handle) {

    }

    void retro_vulkan_unlock_queue_t_impl(void *handle) {

    }

    void retro_vulkan_set_signal_semaphore_t_impl(void *handle, VkSemaphore semaphore) {

    }
}

namespace libRetroRunner {

    VulkanVideoContext::VulkanVideoContext() {
        InitVulkanApi();
        getHWProcAddress = (rr_hardware_render_proc_address_t) &getVKApiAddress;
        screen_width_ = 1;
        screen_height_ = 100;
        is_ready_ = false;
        retro_vk_context_ = nullptr;
        retro_render_interface_ = nullptr;

        window_ = nullptr;

        instance_ = VK_NULL_HANDLE;
        physicalDevice_ = VK_NULL_HANDLE;
        queueFamilyIndex_ = 0;
        logicalDevice_ = VK_NULL_HANDLE;
        graphicQueue_ = VK_NULL_HANDLE;

        commandPool_ = VK_NULL_HANDLE;

        memset(&renderContext_, 0, sizeof(renderContext_));
        destroyDeviceImpl_ = nullptr;

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


        //step: render pass
        //step: others

        //step: create swapchain L:1684
        if (!vulkanCreateSwapchainIfNeeded()) return false;

        //step: call context_reset
        if (false) {
            /*
            AppWindow appWindow = appContext->GetAppWindow();
            screen_width_ = appWindow.width;
            screen_height_ = appWindow.height;
            window_ = appWindow.window;
            if (surface_id_ == appWindow.surfaceId) {
                return false;
            }

            surface_id_ = appWindow.surfaceId;


            //create vk context
            if (vkInstance_ == nullptr) {
                vkInstance_ = new VulkanInstance();
                vkInstance_->setEnableLogger(true);

                const VkApplicationInfo *extVkAppInfo = nullptr;

                if (negotiator && negotiator->get_application_info) {
                    extVkAppInfo = negotiator->get_application_info();
                }
                LOGD_VVC("vulkan instance init: %p", extVkAppInfo);
                if (!vkInstance_->init(extVkAppInfo)) {
                    vkInstance_->destroy();
                    delete vkInstance_;
                    vkInstance_ = nullptr;
                    LOGE_VVC("Failed to init vulkan instance.");
                    return false;
                }
            }

            CHECK_VK_OBJ_NOT_NULL(vkInstance_, "vulkan instance is null.")

            if (vkPipeline_ == nullptr) {
                vkPipeline_ = new VulkanPipeline(vkInstance_->getLogicalDevice());
                if (!vkPipeline_->init(screen_width_, screen_height_)) {
                    delete vkPipeline_;
                    vkPipeline_ = nullptr;
                    LOGE_VVC("Failed to init vulkan pipeline.");
                    return false;
                }
            }


            CHECK_VK_OBJ_NOT_NULL(vkPipeline_, "vulkan pipe line is null.")
            if (vkSwapchain_ == nullptr) {
                vkSwapchain_ = new VulkanSwapchain();
                vkSwapchain_->setInstance(vkInstance_->getInstance());
                vkSwapchain_->setPhysicalDevice(vkInstance_->getPhysicalDevice());
                vkSwapchain_->setLogicalDevice(vkInstance_->getLogicalDevice());
                vkSwapchain_->setQueueFamilyIndex(vkInstance_->getQueueFamilyIndex());
                vkSwapchain_->setRenderPass(vkPipeline_->getRenderPass());
                vkSwapchain_->setCommandPool(vkInstance_->getCommandPool());
                if (!vkSwapchain_->init(window_)) {
                    delete vkSwapchain_;
                    vkSwapchain_ = nullptr;
                    LOGE_VVC("Failed to init vulkan swapchain.");
                    return false;
                }
            }
            CHECK_VK_OBJ_NOT_NULL(vkSwapchain_, "vulkan swapchain is null.")

            if (retro_vk_context_ == nullptr) {
                retro_vk_context_ = new retro_vulkan_context{};
            }

            if (negotiator && negotiator->create_device) {
                static const char *vulkan_device_extensions[] = {
                        "VK_KHR_swapchain",
                };
                int ret = negotiator->create_device(retro_vk_context_,
                                                    vkInstance_->getInstance(),
                                                    vkInstance_->getPhysicalDevice(),
                                                    vkSwapchain_->getSurface(),
                                                    vkGetInstanceProcAddr,
                                                    vulkan_device_extensions, 1,
                                                    nullptr, 0,
                                                    &vkPhysicalDeviceFeatures);
                LOGD_VVC("negotiator create_device ret: %d", ret);
            }


            vulkanIsReady_ = true;

            if (retro_render_interface_ == nullptr)
                retro_render_interface_ = new retro_hw_render_interface_vulkan();

            retro_render_interface_->interface_version = RETRO_HW_RENDER_INTERFACE_VULKAN_VERSION;
            retro_render_interface_->interface_type = RETRO_HW_RENDER_INTERFACE_VULKAN;
            retro_render_interface_->handle = this;
            retro_render_interface_->instance = vkInstance_->getInstance();
            retro_render_interface_->gpu = vkInstance_->getPhysicalDevice();
            retro_render_interface_->device = vkInstance_->getLogicalDevice();

            retro_render_interface_->get_device_proc_addr = vkGetDeviceProcAddr;
            retro_render_interface_->get_instance_proc_addr = vkGetInstanceProcAddr;

            retro_render_interface_->queue = vkInstance_->getGraphicQueue();
            retro_render_interface_->queue_index = vkInstance_->getQueueFamilyIndex();

            */
            //context_reset
            LOGI_VVC("call context_reset now.");
            retro_hw_context_reset_t reset_func = coreCtx->GetRenderHWContextResetCallback();
            if (reset_func) reset_func();

        }
        return true;
    }

    void VulkanVideoContext::Destroy() {
        if (retro_vk_context_) {
            delete retro_vk_context_;
            retro_vk_context_ = nullptr;
        }
    }

    void VulkanVideoContext::Unload() {
        //context_destroy
        vulkanIsReady_ = false;
    }

    void VulkanVideoContext::UpdateVideoSize(unsigned int width, unsigned int height) {
        screen_height_ = height;
        screen_width_ = width;
    }

    void VulkanVideoContext::Prepare() {

    }

    void VulkanVideoContext::OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) {
        if (data && data != RETRO_HW_FRAME_BUFFER_VALID && vulkanIsReady_) {

        }
        if (data)
            DrawFrame();
    }

    void VulkanVideoContext::DrawFrame() {

    }

    bool VulkanVideoContext::TakeScreenshot(const std::string &path) {
        return false;
    }

    bool VulkanVideoContext::getRetroHardwareRenderInterface(void **interface) {
        *interface = retro_render_interface_;
        return retro_render_interface_ != nullptr;
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
        if (surface_ != nullptr && surface_id_ == appWindow.surfaceId) return true;
        window_ = appWindow.window;
        surface_id_ = appWindow.surfaceId;

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

        //surface is new, means swapchain should be rebuild.
        vulkanClearSwapchainIfNeeded();

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
     * because the vulkan implementation in HarmonyOS's ndk is different from Android NDK.
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
                        surface_, vkGetInstanceProcAddr,
                        retro_vulkan_create_device_wrapper_t_impl, this);
                if (!createResult) {
                    LOGW_VVC("Failed to create device (2) on provided GPU device, now let the core choose.");
                    createResult = negotiation->create_device2(
                            &context, instance_, VK_NULL_HANDLE,
                            surface_, vkGetInstanceProcAddr,
                            retro_vulkan_create_device_wrapper_t_impl, this);
                }
            } else {
                std::vector<const char *> deviceExtensions{};
                deviceExtensions.push_back("VK_KHR_swapchain");
                LOGD_VVC("create device with our selected gpu.");
                createResult = negotiation->create_device(
                        &context, instance_, physicalDevice_, surface_, vkGetInstanceProcAddr,
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
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, idx, surface_, &supported);
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
        vkDeviceWaitIdle(logicalDevice_);
        //todo: remove swapchain image fences

        VkSurfaceCapabilitiesKHR surfaceProperties;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &surfaceProperties);

        //skip to create swapchain when the surface size is 0
        if (surfaceProperties.currentExtent.width == 0 || surfaceProperties.currentExtent.height == 0) {
            return false;
        }


        return false;
    }

    bool VulkanVideoContext::vulkanClearSwapchainIfNeeded() {
        return true;
    }

    bool VulkanVideoContext::vulkanCreateRenderContextIfNeeded() {
        if (renderContext_.valid) return true;
        return true;
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
