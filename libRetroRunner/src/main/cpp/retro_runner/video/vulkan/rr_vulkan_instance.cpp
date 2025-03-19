//
// Created by aidoo .
//
#include "rr_vulkan_instance.h"
#include <vulkan/vulkan.h>

namespace libRetroRunner {


    static void logAvailableLayers() {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        LOGD_VC("Available Vulkan Layers:");
        for (const auto &layer: availableLayers) {
            LOGD_VC(" - %s", layer.layerName);
        }
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData) {

        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            LOGD_VC("[WARN]: %s", pCallbackData->pMessage);
        else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            LOGD_VC("[ERR]: %s", pCallbackData->pMessage);
        return VK_FALSE;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallbackFn(
            VkDebugReportFlagsEXT flags,
            VkDebugReportObjectTypeEXT objectType,
            uint64_t object,
            size_t location,
            int32_t messageCode,
            const char *pLayerPrefix,
            const char *pMessage,
            void *pUserData) {
        if (pMessage) {
            LOGD_VC("[LOG][%d]: %s", messageCode, pMessage);
        }
        /*
        if (messageCode == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            LOGD_VC("[WARN]: %s", pMessage);
        else if (messageCode == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            LOGD_VC("[ERR]: %s", pMessage);
            */
        return VK_FALSE;
    }

    bool isPhysicalDeviceSuitable(VkPhysicalDevice device, int *graphicsFamily) {
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

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, VkDebugUtilsMessengerEXT *pMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        return func ? func(instance, pCreateInfo, nullptr, pMessenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, VkDebugReportCallbackEXT *pCallback) {
        auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
        return func ? func(instance, pCreateInfo, nullptr, pCallback) : VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) func(instance, messenger, nullptr);
    }

    void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback) {
        auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
        if (func) func(instance, callback, nullptr);
    }

}

namespace libRetroRunner {

    VulkanInstance::VulkanInstance() {
        allReady = false;
        enableLogger_ = false;
    }

    VulkanInstance::~VulkanInstance() {
        destroy();
    }

    void VulkanInstance::logAvailableExtensions() {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        LOGD_VC("available extensions:");
        for (const auto &ext: extensions) {

            if (enableLogger_ && strstr(ext.extensionName, "debug")) {
                extensions_.push_back(ext.extensionName);
                LOGD_VC(" - %s [selected]", ext.extensionName);
            } else {
                LOGD_VC(" - %s", ext.extensionName);
            }
        }
    }

    bool VulkanInstance::createInstance() {
        if (instance_) {
            LOGW_VC("Vulkan instance already created.");
            return true;
        }

        extensions_.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef  ANDROID
        extensions_.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
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
                .enabledExtensionCount = static_cast<uint32_t>(extensions_.size()),
                .ppEnabledExtensionNames = extensions_.data()
        };
        /*
        std::vector<const char *> validation_layers{
                "VK_LAYER_KHRONOS_validation"
        };
        if (vulkanContext.enableValidations) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            createInfo.ppEnabledLayerNames = validation_layers.data();
        }*/

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance_);
        if (result != VK_SUCCESS) {
            LOGE_VC("create instance failed. %d", result);
            return false;
        }
        LOGD_VC("create instance :\t%p", instance_);
        return true;
    }

    bool VulkanInstance::createLogger() {
        if (!enableLogger_) return true;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = &debugCallback;

        if (CreateDebugUtilsMessengerEXT(instance_, &debugCreateInfo, &debugMessenger_) != VK_SUCCESS) {
            VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo{};
            debugReportCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            debugReportCreateInfo.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            debugReportCreateInfo.pfnCallback = debugReportCallbackFn;

            if (CreateDebugReportCallbackEXT(instance_, &debugReportCreateInfo, &debugCallback_) != VK_SUCCESS) {
                LOGE_VC("failed to setup vulkan debug message callback.");
                return false;
            } else {
                LOGD_VC("debug callback:\t%p", debugCallback_);
            }
        } else {
            LOGD_VC("debug messenger:\t%p", debugMessenger_);
        }
        return true;
    }

    bool VulkanInstance::choosePhysicalDevice() {
        if (physicalDevice_) {
            LOGW_VC("Physical device already chosen.");
            return true;
        }
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
        if (deviceCount == 0) {
            LOGE_VC("Can't find any gpu for vulkan.");
            return false;
        }
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        VkResult result = vkEnumeratePhysicalDevices(instance_, &deviceCount, physicalDevices.data());
        if (result != VK_SUCCESS) {
            LOGE_VC("Error finding physical device.");
            return false;
        }

        //TODO:Better way to choose physical device?
        for (const auto &device: physicalDevices) {
            int graphicsFamily = -1;
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            if ((deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
                 deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) &&
                isPhysicalDeviceSuitable(device, &graphicsFamily)) {
                physicalDevice_ = device;
                queueFamilyIndex_ = graphicsFamily;
                LOGD_VC("physical device:\t%s", deviceProperties.deviceName);
                LOGD_VC("graphics family:\t%d", graphicsFamily);
                break;
            }
        }
        if (physicalDevice_ == VK_NULL_HANDLE) {
            LOGE_VC("Can not find any suitable physical device.\n");
            return false;
        }

        return true;
    }

    bool VulkanInstance::createLogicalDevice() {
        if (logicalDevice_) {
            LOGW_VC("Logical device already created.");
            return true;
        }
        const std::vector<const char *> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = static_cast<uint32_t >(queueFamilyIndex_),
                .queueCount = 1,
                .pQueuePriorities = &queuePriority
        };
        //TODO:调节逻辑设备的功能
        VkPhysicalDeviceFeatures deviceFeatures = {};
        VkDeviceCreateInfo deviceCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &queueCreateInfo,
                .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
                .ppEnabledExtensionNames = deviceExtensions.data(),
                .pEnabledFeatures = &deviceFeatures,
        };
        VkResult result = vkCreateDevice(physicalDevice_, &deviceCreateInfo, nullptr, &logicalDevice_);
        if (result != VK_SUCCESS) {
            LOGE_VC("failed to created logical device: %d", result);
            return false;
        }
        LOGD_VC("logical device:\t%p", logicalDevice_);

        vkGetDeviceQueue(logicalDevice_, queueFamilyIndex_, 0, &graphicQueue_);
        LOGD_VC("graphics queue:\t%p", graphicQueue_);

        return true;
    }

    bool VulkanInstance::init() {
        if (allReady) return true;

        extensions_.clear();
        extensions_.resize(0);
        if (enableLogger_) {
            logAvailableExtensions();
            logAvailableLayers();
        }
        if (!createInstance()) return false;
        createLogger();
        if (!choosePhysicalDevice()) return false;
        if (!createLogicalDevice()) return false;
        if (!createCommandPool()) return false;
        allReady = true;
        return allReady;
    }

    void VulkanInstance::destroy() {
        if (instance_ != VK_NULL_HANDLE) {
            if (logicalDevice_) {
                vkDestroyDevice(logicalDevice_, nullptr);
                logicalDevice_ = VK_NULL_HANDLE;
            }
            if (debugMessenger_) {
                DestroyDebugUtilsMessengerEXT(instance_, debugMessenger_);
                debugMessenger_ = VK_NULL_HANDLE;
            }
            if (debugCallback_) {
                DestroyDebugReportCallbackEXT(instance_, debugCallback_);
                debugCallback_ = VK_NULL_HANDLE;
            }
            vkDestroyInstance(instance_, nullptr);
            instance_ = VK_NULL_HANDLE;
            allReady = false;
        }

    }

    bool VulkanInstance::createCommandPool() {
        if (commandPool_) {
            LOGW_VC("Command pool already created.");
            return true;
        }
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
}