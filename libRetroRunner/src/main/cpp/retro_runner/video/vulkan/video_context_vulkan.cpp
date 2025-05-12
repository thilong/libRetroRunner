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

#define VVC_ASSERT_RETURN(_CON_, _MSG_, _RET_VALUE_) \
    if (!(_CON_)) {          \
        LOGE_VVC(_MSG_);        \
        return _RET_VALUE_;       \
    }

#define VVC_ASSERT(_CON_, _MSG_) \
    if (!(_CON_)) {          \
        LOGE_VVC(_MSG_);        \
        return;       \
    }


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
        screen_width_ = 1;
        screen_height_ = 100;
        is_ready_ = false;
        retro_vk_context_ = nullptr;
        vk_instance_ = VK_NULL_HANDLE;
        vk_physical_device_ = VK_NULL_HANDLE;
        vk_surface_ = VK_NULL_HANDLE;
        retro_render_interface_ = nullptr;
    }

    VulkanVideoContext::~VulkanVideoContext() {
        Destroy();
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

        //create vk context

        //context_reset

        /*
        createVkInstance();
        VVC_ASSERT_RETURN(vk_instance_ != VK_NULL_HANDLE, "vulkan instance should not be null.", false);
        selectVkPhysicalDevice();
        VVC_ASSERT_RETURN(vk_physical_device_ != VK_NULL_HANDLE, "no suitable gpu.", false);
        createVkSurface();
        VVC_ASSERT_RETURN(vk_surface_ != VK_NULL_HANDLE, "no surface found.", false)

        if (!retro_vk_context_) {
            retro_vk_context_ = new retro_vulkan_context();
        }

        auto iNegotiation = getNegotiationInterface();

        if (iNegotiation && iNegotiation->create_device) {
            LOGD_VVC("negotiation: %p", vkGetInstanceProcAddr(vk_instance_, "vkGetDeviceProcAddr"));
            bool createDeviceResult = iNegotiation->create_device(retro_vk_context_, vk_instance_, vk_physical_device_, vk_surface_,
                                                                  vkGetInstanceProcAddr,
                                                                  vk_extensions_.data(), vk_extensions_.size(),
                                                                  nullptr, 0, &vk_physical_device_feature_);
            if (!createDeviceResult) {
                LOGE_VVC("[NEGOTIATION] failed to create device");
                return false;
            }
            vulkanIsReady_ = true;
            return true;
        }
         */
        vulkanIsReady_ = false;
        return false;
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

    void VulkanVideoContext::createVkInstance() {
        if (vk_instance_) return;
        vk_extensions_.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef ANDROID
        vk_extensions_.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif
        VkApplicationInfo appInfo{
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName = "libRetroRunner",
                .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                .pEngineName = "No Engine",
                .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                .apiVersion = VK_API_VERSION_1_0,
        };
        VkInstanceCreateInfo createInfo{
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pApplicationInfo = &appInfo,
                .enabledExtensionCount = static_cast<uint32_t>(vk_extensions_.size()),
                .ppEnabledExtensionNames = vk_extensions_.data()
        };
        auto iNegotiation = getNegotiationInterface();

        if (iNegotiation && iNegotiation->get_application_info) {
            auto negotiationAppInfo = iNegotiation->get_application_info();
            createInfo.pApplicationInfo = negotiationAppInfo;
        }
        VkResult createResult = vkCreateInstance(&createInfo, nullptr, &vk_instance_);
        if (createResult != VK_SUCCESS || vk_instance_ == nullptr) {
            LOGE_VVC("failed to create vulkan instance: %d", createResult);
        } else {
            LOGD_VVC("Instance:\t\t%p", vk_instance_);
        }
    }

    bool VulkanVideoContext::isPhysicalDeviceSuitable(VkPhysicalDevice device, int *graphicsFamily) {
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

    void VulkanVideoContext::selectVkPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(vk_instance_, &deviceCount, nullptr);
        VVC_ASSERT(deviceCount != 0, "Can't find any gpu for vulkan.")

        LOGD_VVC("gpu device count: %d", deviceCount);

        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        VkResult result = vkEnumeratePhysicalDevices(vk_instance_, &deviceCount, physicalDevices.data());
        if (result != VK_SUCCESS) {
            LOGE_VVC("Error finding physical device:%d", result);
            return;
        }

        //TODO:Better way to choose physical device?
        for (const auto &device: physicalDevices) {
            int graphicsFamily = -1;
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            if ((deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
                 deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) &&
                isPhysicalDeviceSuitable(device, &graphicsFamily)) {
                vk_physical_device_ = device;
                LOGD_VVC("physical device:\t%s", deviceProperties.deviceName);
                //LOGD_VC("graphics family:\t%d", graphicsFamily);
                break;
            }
        }
        VVC_ASSERT(vk_physical_device_ != VK_NULL_HANDLE, "Can not find any suitable physical device.\n")
    }

    void VulkanVideoContext::createVkSurface() {
        if (vk_surface_) return;

        auto app = AppContext::Current();
        auto appWindow = app->GetAppWindow();
        VkAndroidSurfaceCreateInfoKHR createInfo{
                .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .window = (ANativeWindow *) appWindow.window
        };
        VkResult createRet = vkCreateAndroidSurfaceKHR(vk_instance_, &createInfo, nullptr, &vk_surface_);
        if (createRet != VK_SUCCESS || vk_surface_ == VK_NULL_HANDLE) {
            LOGE_VC("Surface create failed:  %d\n", createRet);
        } else {
            LOGD_VC("Surface KHR:\t\t%p\n", vk_surface_);
        }


    }


}
