//
// Created by aidoo.
//

#ifndef LIBRETRORUNNER_RR_VULKAN_INSTANCE_H
#define LIBRETRORUNNER_RR_VULKAN_INSTANCE_H

#include "rr_vulkan.h"
#include <libretro-common/include/libretro_vulkan.h>

namespace libRetroRunner {
    class VulkanInstance {
    public:
        VulkanInstance();

        ~VulkanInstance();


        bool createInstance();

        bool createLogger();

        bool choosePhysicalDevice();

        bool createLogicalDevice();
        bool createCommandPool();
        bool init();

        void destroy();

    private:
        void logAvailableExtensions();

    public:
        inline VkInstance getInstance() const { return instance_; }

        inline VkDebugUtilsMessengerEXT getDebugMessenger() const { return debugMessenger_; }

        inline VkPhysicalDevice getPhysicalDevice() const { return physicalDevice_; }

        inline VkDevice getLogicalDevice() const { return logicalDevice_; }

        inline uint32_t getQueueFamilyIndex() const { return queueFamilyIndex_; }

        inline VkQueue getGraphicQueue() const { return graphicQueue_; }

        inline VkCommandPool getCommandPool() const { return commandPool_; }

        inline void setEnableLogger(bool enableLogger) { enableLogger_ = enableLogger; }

    private:
        VkInstance instance_{};

        VkPhysicalDevice physicalDevice_{};
        VkDevice logicalDevice_{};
        uint32_t queueFamilyIndex_{};
        VkQueue graphicQueue_{};
        VkCommandPool commandPool_{};

        bool enableLogger_{};
        bool allReady{};

        VkDebugUtilsMessengerEXT debugMessenger_{};
        VkDebugReportCallbackEXT debugCallback_{};

        std::vector<const char *> extensions_{};

    };
}


#endif //LIBRETRORUNNER_RR_VULKAN_INSTANCE_H
