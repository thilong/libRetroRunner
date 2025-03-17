//
// Created by aidoo on 3/3/2025.
//
#include <vector>
#include "vulkan_context.h"
#include "../../types/log.h"
#include "vulkan_context.h"

#define LOGD_VVC(...) LOGD("[Vulkan] " __VA_ARGS__)
#define LOGW_VVC(...) LOGW("[Vulkan] " __VA_ARGS__)
#define LOGE_VVC(...) LOGE("[Vulkan] " __VA_ARGS__)
#define LOGI_VVC(...) LOGI("[Vulkan] " __VA_ARGS__)

namespace libRetroRunner {

    const uint32_t *shader_vert_spv = nullptr;
    size_t shader_vert_spv_size = 0;
    const uint32_t *shader_frag_spv = nullptr;
    size_t shader_frag_spv_size = 0;

    VulkanContext::VulkanContext() {

    }

    VulkanContext::~VulkanContext() {

    }

    void VulkanContext::Destroy() {

    }

    //========================

    void VulkanContext::logVkExtensions() {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        LOGD_VVC("Available extensions:");
        for (const auto &extension: extensions) {
            LOGD_VVC("\t%s", extension.extensionName);
        }
    }

    void VulkanContext::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
        LOGW_VVC("%s", pCallbackData->pMessage);
    }

    void VulkanContext::setVkImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStages, VkPipelineStageFlags destStages) {
        VkImageMemoryBarrier imageMemoryBarrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = 0,
                .dstAccessMask = 0,
                .oldLayout = oldImageLayout,
                .newLayout = newImageLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = image,
                .subresourceRange =
                        {
                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                .baseMipLevel = 0,
                                .levelCount = 1,
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                        },
        };

        switch (oldImageLayout) {
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                break;

            default:
                break;
        }

        switch (newImageLayout) {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                imageMemoryBarrier.dstAccessMask =
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                break;

            default:
                break;
        }
        vkCmdPipelineBarrier(cmdBuffer, srcStages, destStages, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);

    }

    //========================

    bool VulkanContext::createInstance() {
        VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback = reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(&VulkanContext::debugCallback);
        VkApplicationInfo appInfo{
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pNext = nullptr,
                .pApplicationName = "RetroRunner",
                .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                .pEngineName = "RetroRunner",
                .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                .apiVersion = VK_API_VERSION_1_0
        };
        std::vector<const char *> requiredExtensions = {
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
        };
        VkInstanceCreateInfo instanceCreateInfo{
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pNext = &debugInfo,
                .flags = 0,
                .pApplicationInfo = &appInfo,
                .enabledLayerCount = 0,
                .ppEnabledLayerNames = nullptr,
                .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
                .ppEnabledExtensionNames = requiredExtensions.data(),
        };
        //instanceCreateInfo.pNext = &debugInfo;
        VkResult createVkInstanceResult = vkCreateInstance(&instanceCreateInfo, nullptr, &(device_info_.instance));
        if (createVkInstanceResult != VK_SUCCESS || device_info_.instance == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to create vulkan instance! %d", createVkInstanceResult);
            return false;
        } else {
            LOGD_VVC("Vulkan instance created: %p", device_info_.instance);
        }
        return true;
    }

    bool VulkanContext::findPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(device_info_.instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            LOGE_VVC("Failed to find GPUs with Vulkan support!");
            return false;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(device_info_.instance, &deviceCount, devices.data());

        for (const auto &device: devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
                deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                device_info_.physical_device = device;
                break;
            }
        }

        if (device_info_.physical_device == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to find a suitable GPU!");
            return false;
        } else {
            LOGD_VVC("Physical device found: %p", device_info_.physical_device);
        }

        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(device_info_.physical_device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device_info_.physical_device, &queueFamilyCount, queueFamilyProperties.data());

        uint32_t queueFamilyIndex;
        for (queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++) {
            if (queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                break;
            }
        }
        if (queueFamilyIndex >= queueFamilyCount) {
            LOGE_VVC("Failed to find a suitable queue family!");
            return false;
        } else {
            LOGD_VVC("Queue family found: %d", queueFamilyIndex);
            device_info_.queue_family_index = queueFamilyIndex;
        }
        return true;
    }

    bool VulkanContext::createLogicalDevice() {
        float queuePriority = 1.0f;
        std::vector<const char *> device_extensions;
        device_extensions.push_back("VK_KHR_swapchain");

        VkDeviceQueueCreateInfo queueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .queueFamilyIndex = device_info_.queue_family_index,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
        };

        VkDeviceCreateInfo deviceCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = nullptr,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &queueCreateInfo,
                .enabledLayerCount = 0,
                .ppEnabledLayerNames = nullptr,
                .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
                .ppEnabledExtensionNames = device_extensions.data(),
                .pEnabledFeatures = nullptr,
        };

        VkResult createDeviceResult = vkCreateDevice(device_info_.physical_device, &deviceCreateInfo, nullptr, &device_info_.logical_device);

        if (createDeviceResult != VK_SUCCESS || device_info_.logical_device == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to create logical device!");
            return false;
        } else {
            LOGD_VVC("Logical device created: %p", device_info_.logical_device);
        }


        vkGetDeviceQueue(device_info_.logical_device, device_info_.queue_family_index, 0, &device_info_.queue);
        if (device_info_.queue == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to get graphics queue!");
            return false;
        } else {
            LOGD_VVC("Graphics queue choose: %p", &device_info_.queue);
        }
        return true;
    }

    //========================

    bool VulkanContext::attachWindow(void *window) {
        VkResult createSurfaceResult = VK_NOT_READY;
#ifdef ANDROID
        VkAndroidSurfaceCreateInfoKHR createInfo{
                .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .window = static_cast<ANativeWindow *>(window)};

        createSurfaceResult = vkCreateAndroidSurfaceKHR(device_info_.instance, &createInfo, nullptr, &(swapchain_info_.surface));
#endif
        if (createSurfaceResult != VK_SUCCESS || swapchain_info_.surface == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to create surface! %d", createSurfaceResult);
            return false;
        } else {
            LOGD_VVC("Surface created: %p", swapchain_info_.surface);
        }
        return true;
    }

    bool VulkanContext::createSwapchain() {
        LOGD_VVC("Creating swap chain and images...");

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_info_.physical_device, swapchain_info_.surface, &surfaceCapabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device_info_.physical_device, swapchain_info_.surface, &formatCount, nullptr);

        if (formatCount == 0) {
            LOGE_VVC("Failed to get surface formats!");
            return false;
        } else {
            LOGD_VVC("Available Surface formats: total %d", formatCount);
        }

        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device_info_.physical_device, swapchain_info_.surface, &formatCount, formats.data());
        VkSurfaceFormatKHR surfaceFormat = formats[0];

        uint32_t chosenFormat;
        for (chosenFormat = 0; chosenFormat < formatCount; chosenFormat++) {
            if (formats[chosenFormat].format == VK_FORMAT_R8G8B8A8_UNORM) break;
        }
        if (chosenFormat >= formatCount) {
            LOGE_VVC("Failed to find suitable format!");
            return false;
        }
        swapchain_info_.display_extent = surfaceCapabilities.currentExtent;
        swapchain_info_.display_format = formats[chosenFormat].format;
        swapchain_info_.color_space = formats[chosenFormat].colorSpace;

        LOGD_VVC("Swapchain display size: %d, %d, format: %d, color space: %d", swapchain_info_.display_extent.width, swapchain_info_.display_extent.height, swapchain_info_.display_format, swapchain_info_.color_space);

        VkSwapchainCreateInfoKHR swapchainCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = nullptr,
                .surface = swapchain_info_.surface,
                .minImageCount = surfaceCapabilities.minImageCount,
                .imageFormat = swapchain_info_.display_format,
                .imageColorSpace = swapchain_info_.color_space,
                .imageExtent = surfaceCapabilities.currentExtent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &device_info_.queue_family_index,
                .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                .presentMode = VK_PRESENT_MODE_FIFO_KHR,
                .clipped = VK_FALSE,
                .oldSwapchain = VK_NULL_HANDLE, //注意这里，是否要在下一次创建的时候释放掉之前的swapchain
        };
        VkResult createSwapChainResult = vkCreateSwapchainKHR(device_info_.logical_device, &swapchainCreateInfo, nullptr, &swapchain_info_.swapchain);

        if (createSwapChainResult || swapchain_info_.swapchain == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to create swap chain!");
            return false;
        } else {
            LOGD_VVC("Vulkan swap chain created: %p", swapchain_info_.swapchain);
        }

        vkGetSwapchainImagesKHR(device_info_.logical_device, swapchain_info_.swapchain, &swapchain_info_.image_count, nullptr);
        LOGD_VVC("Swapchain image created, total: %d", swapchain_info_.image_count);
        return true;
    }

    bool VulkanContext::createImageView() {
        if (swapchain_info_.image_count == 0) {
            LOGE_VVC("Swapchain image length is 0!");
            return false;
        }
        swapchain_info_.images.resize(swapchain_info_.image_count);
        vkGetSwapchainImagesKHR(device_info_.logical_device, swapchain_info_.swapchain, &swapchain_info_.image_count, swapchain_info_.images.data());

        swapchain_info_.image_views.resize(swapchain_info_.image_count);

        int succeedCount = 0;
        for (uint32_t i = 0; i < swapchain_info_.image_count; i++) {
            VkImageViewCreateInfo viewCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .image = swapchain_info_.images[i],
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = swapchain_info_.display_format,
                    .components =
                            {
                                    .r = VK_COMPONENT_SWIZZLE_R,
                                    .g = VK_COMPONENT_SWIZZLE_G,
                                    .b = VK_COMPONENT_SWIZZLE_B,
                                    .a = VK_COMPONENT_SWIZZLE_A,
                            },
                    .subresourceRange =
                            {
                                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                    .baseMipLevel = 0,
                                    .levelCount = 1,
                                    .baseArrayLayer = 0,
                                    .layerCount = 1,
                            },
            };
            if (vkCreateImageView(device_info_.logical_device, &viewCreateInfo, nullptr, &swapchain_info_.image_views[i]) != VK_SUCCESS || swapchain_info_.image_views[i] == VK_NULL_HANDLE) {
                LOGE_VVC("Failed to create swapchain image view %d!", i);
            } else {
                succeedCount++;
            }
        }
        if (succeedCount == swapchain_info_.image_count) {
            LOGD_VVC("Swapchain image views created: total %d", succeedCount);
            return true;
        } else {
            LOGD_VVC("Swapchain image views create failed: %d/%d", succeedCount, swapchain_info_.image_count);
            return false;
        }
    }

    bool VulkanContext::createRenderPass() {
        VkAttachmentDescription attachmentDescriptions{
                .format = swapchain_info_.display_format,
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
        VkResult createRenderPassResult = vkCreateRenderPass(device_info_.logical_device, &renderPassCreateInfo, nullptr, &render_info_.render_pass);

        if (createRenderPassResult || render_info_.render_pass == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to create render pass!");
            return false;
        } else {
            LOGD_VVC("Render pass created: %p", render_info_.render_pass);
        }
        return true;
    }

    bool VulkanContext::createVkShaderModule(const uint32_t *content, size_t size, VulkanShaderType type, VkShaderModule *shaderOut) const {
        VkShaderModuleCreateInfo shaderModuleCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = size,
                .pCode = content,
        };
        VkResult result = vkCreateShaderModule(device_info_.logical_device, &shaderModuleCreateInfo, nullptr, shaderOut);
        if (result != VK_SUCCESS || *shaderOut == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to create shader module! %d", result);
            return false;
        } else {
            LOGD_VVC("Shader module created: %p", *shaderOut);
            return true;
        }

    }

    bool VulkanContext::createVkGraphicsPipeline() {
        int sizeShift = sizeof(uint32_t) / sizeof(char);
        VkShaderModule vkVertShaderModule_ = VK_NULL_HANDLE;
        VkShaderModule vkFragShaderModule_ = VK_NULL_HANDLE;
        bool ret = false;
        do {
            if (!createVkShaderModule(reinterpret_cast<const uint32_t *>(shader_vert_spv), shader_vert_spv_size, SHADER_VERTEX, &vkVertShaderModule_)) {
                break;
            }
            if (!createVkShaderModule(reinterpret_cast<const uint32_t *>(shader_frag_spv), shader_frag_spv_size, SHADER_FRAGMENT, &vkFragShaderModule_)) {
                break;
            }

            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                    .pNext = nullptr,
                    .setLayoutCount = 0,
                    .pSetLayouts = nullptr,             // 绑定的描述符集布局
                    .pushConstantRangeCount = 0,
                    .pPushConstantRanges = nullptr,     // 推送常量范围
            };
            VkResult createPipelineLayoutResult = vkCreatePipelineLayout(device_info_.logical_device, &pipelineLayoutCreateInfo, nullptr, &pipeline_info_.layout);
            if (createPipelineLayoutResult != VK_SUCCESS || pipeline_info_.layout == VK_NULL_HANDLE) {
                LOGE_VVC("Failed to create pipeline layout! %d", createPipelineLayoutResult);
                break;
            }

            VkPipelineShaderStageCreateInfo shaderStages[2]{
                    {
                            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                            .pNext = nullptr,
                            .flags = 0,
                            .stage = VK_SHADER_STAGE_VERTEX_BIT,
                            .module = vkVertShaderModule_,
                            .pName = "main",
                            .pSpecializationInfo = nullptr,
                    },
                    {
                            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                            .pNext = nullptr,
                            .flags = 0,
                            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                            .module = vkFragShaderModule_,
                            .pName = "main",
                            .pSpecializationInfo = nullptr,
                    }};

            //设置视口， surface 发生变化后使用vkCmdSetViewport设置
            VkViewport viewports{
                    .x = 0,
                    .y = 0,
                    .width = (float) swapchain_info_.display_extent.width,
                    .height = (float) swapchain_info_.display_extent.height,
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f,
            };
            //设置裁剪区域, surface 发生变化后使用vkCmdSetScissor设置
            VkRect2D scissor = {
                    .offset {.x = 0, .y = 0,},
                    .extent = swapchain_info_.display_extent,
            };
            //设置视口和裁剪区域
            VkPipelineViewportStateCreateInfo viewportInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .viewportCount = 1,
                    .pViewports = &viewports,
                    .scissorCount = 1,
                    .pScissors = &scissor,
            };

            // Specify multisample info
            VkSampleMask sampleMask = ~0u;
            VkPipelineMultisampleStateCreateInfo multiSampleInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                    .sampleShadingEnable = VK_FALSE,
                    .minSampleShading = 0,
                    .pSampleMask = &sampleMask,
                    .alphaToCoverageEnable = VK_FALSE,
                    .alphaToOneEnable = VK_FALSE,
            };

            // Specify color blend state
            VkPipelineColorBlendAttachmentState attachmentStates{
                    .blendEnable = VK_FALSE,
                    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            };
            VkPipelineColorBlendStateCreateInfo colorBlendInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .logicOpEnable = VK_FALSE,
                    .logicOp = VK_LOGIC_OP_COPY,
                    .attachmentCount = 1,
                    .pAttachments = &attachmentStates,
            };

            // Specify rasterizer info
            VkPipelineRasterizationStateCreateInfo rasterInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .depthClampEnable = VK_FALSE,
                    .rasterizerDiscardEnable = VK_FALSE,
                    .polygonMode = VK_POLYGON_MODE_FILL,
                    .cullMode = VK_CULL_MODE_NONE,
                    .frontFace = VK_FRONT_FACE_CLOCKWISE,
                    .depthBiasEnable = VK_FALSE,
                    .lineWidth = 1,
            };

            // Specify input assembler state
            VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                    .primitiveRestartEnable = VK_FALSE,
            };

            // Specify vertex input state
            VkVertexInputBindingDescription vertex_input_bindings{
                    .binding = 0,
                    .stride = 3 * sizeof(float),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            };
            VkVertexInputAttributeDescription vertex_input_attributes[1]{{
                                                                                 .location = 0,
                                                                                 .binding = 0,
                                                                                 .format = VK_FORMAT_R32G32B32_SFLOAT,
                                                                                 .offset = 0,
                                                                         }};
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .vertexBindingDescriptionCount = 1,
                    .pVertexBindingDescriptions = &vertex_input_bindings,
                    .vertexAttributeDescriptionCount = 1,
                    .pVertexAttributeDescriptions = vertex_input_attributes,
            };

            // Create the pipeline cache
            VkPipelineCacheCreateInfo pipelineCacheInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,  // reserved, must be 0
                    .initialDataSize = 0,
                    .pInitialData = nullptr,
            };

            VkResult createPipelineCacheResult = vkCreatePipelineCache(device_info_.logical_device, &pipelineCacheInfo, nullptr, &pipeline_info_.cache);
            if (createPipelineCacheResult != VK_SUCCESS || pipeline_info_.cache == VK_NULL_HANDLE) {
                LOGE_VVC("Failed to create pipeline cache! %d", createPipelineCacheResult);
                break;
            } else {
                LOGD_VVC("Pipeline cache created: %p", pipeline_info_.cache);
            }

            // Create the pipeline
            VkGraphicsPipelineCreateInfo pipelineCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stageCount = 2,
                    .pStages = shaderStages,
                    .pVertexInputState = &vertexInputInfo,
                    .pInputAssemblyState = &inputAssemblyInfo,
                    .pTessellationState = nullptr,
                    .pViewportState = &viewportInfo,
                    .pRasterizationState = &rasterInfo,
                    .pMultisampleState = &multiSampleInfo,
                    .pDepthStencilState = nullptr,
                    .pColorBlendState = &colorBlendInfo,
                    .pDynamicState = nullptr,
                    .layout = pipeline_info_.layout,
                    .renderPass = render_info_.render_pass,
                    .subpass = 0,
                    .basePipelineHandle = VK_NULL_HANDLE,
                    .basePipelineIndex = 0,
            };

            VkResult createGraphicsPipelinesResult = vkCreateGraphicsPipelines(
                    device_info_.logical_device, pipeline_info_.cache, 1, &pipelineCreateInfo, nullptr,
                    &pipeline_info_.pipeline);

            if (createGraphicsPipelinesResult != VK_SUCCESS || pipeline_info_.pipeline == VK_NULL_HANDLE) {
                LOGE_VVC("Failed to create graphics pipeline! %d", createGraphicsPipelinesResult);
                break;
            } else {
                LOGD_VVC("Graphics pipeline created: %p", pipeline_info_.pipeline);
            }
            ret = true;
        } while (false);

        if (vkVertShaderModule_ != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device_info_.logical_device, vkVertShaderModule_, nullptr);
        }
        if (vkFragShaderModule_ != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device_info_.logical_device, vkFragShaderModule_, nullptr);
        }
        return ret;
    }

    bool VulkanContext::createVkCommandPool() {
        VkCommandPoolCreateInfo cmdPoolCreateInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = device_info_.queue_family_index,
        };
        VkResult createCmdPoolResult = vkCreateCommandPool(device_info_.logical_device, &cmdPoolCreateInfo, nullptr, &render_info_.command_pool);

        if (createCmdPoolResult != VK_SUCCESS || render_info_.command_pool == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to create command pool! %d", createCmdPoolResult);
            return false;
        } else {
            LOGD_VVC("Command pool created: %p", render_info_.command_pool);
        }
        return true;

    }

    bool VulkanContext::createVkCommandBuffers() {
        // Record a command buffer that just clear the screen
        // 1 command buffer draw in 1 framebuffer
        // In our case we need 2 command as we have 2 framebuffer
        render_info_.command_buffer_len = swapchain_info_.image_count;
        render_info_.command_buffer = new VkCommandBuffer[swapchain_info_.image_count];
        VkCommandBufferAllocateInfo cmdBufferCreateInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = render_info_.command_pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = render_info_.command_buffer_len
        };
        VkResult createCmdBufferResult = vkAllocateCommandBuffers(device_info_.logical_device, &cmdBufferCreateInfo, render_info_.command_buffer);
        if (createCmdBufferResult != VK_SUCCESS) {
            LOGE_VVC("Failed to create command buffer! %d", createCmdBufferResult);
            return false;
        }


        //draw
        for (int bufferIndex = 0; bufferIndex < render_info_.command_buffer_len; bufferIndex++) {
            // We start by creating and declare the "beginning" our command buffer
            VkCommandBufferBeginInfo cmdBufferBeginInfo{
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .pInheritanceInfo = nullptr,
            };
            if (vkBeginCommandBuffer(render_info_.command_buffer[bufferIndex], &cmdBufferBeginInfo) != VK_SUCCESS) {
                LOGE_VVC("Failed to begin command buffer!");
                return false;
            }
            // transition the display image to color attachment layout
            setVkImageLayout(render_info_.command_buffer[bufferIndex],
                             swapchain_info_.images[bufferIndex],
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

            // Now we start a renderpass. Any draw command has to be recorded in a
            // renderpass
            VkClearValue clearVals{.color {.float32 {0.0f, 0.34f, 0.90f, 1.0f}}};
            VkRenderPassBeginInfo renderPassBeginInfo{
                    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                    .pNext = nullptr,
                    .renderPass = render_info_.render_pass,
                    .framebuffer = swapchain_info_.frame_buffers[bufferIndex],
                    .renderArea = {
                            .offset {
                                    .x = 0,
                                    .y = 0,
                            },
                            .extent = swapchain_info_.display_extent},
                    .clearValueCount = 1,
                    .pClearValues = &clearVals};
            vkCmdBeginRenderPass(render_info_.command_buffer[bufferIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            //TODO:在这里提交了绘制命令
            /*
            // Bind what is necessary to the command buffer
            vkCmdBindPipeline(render_info_.command_buffer[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info_.pipeline);
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(render_info_.command_buffer[bufferIndex], 0, 1, &bufferInfos_.vertexBuf_, &offset);

            // Draw Triangle
            vkCmdDraw(renderInfo_.cmdBuffer_[bufferIndex], 3, 1, 0, 0);
            */
            vkCmdEndRenderPass(render_info_.command_buffer[bufferIndex]);

            if (vkEndCommandBuffer(render_info_.command_buffer[bufferIndex]) != VK_SUCCESS) {
                LOGE_VVC("Failed to end command buffer!");
                return false;
            }
        }

        return true;
    }

    bool VulkanContext::createVkFrameBuffers() {
        swapchain_info_.frame_buffers.resize(swapchain_info_.image_count);
        int succeedCount = 0;
        for (size_t i = 0; i < swapchain_info_.image_count; i++) {
            VkImageView attachments[] = {swapchain_info_.image_views[i]};
            VkFramebufferCreateInfo fbCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .pNext = nullptr,
                    .renderPass = render_info_.render_pass,
                    .attachmentCount = 1,  // TODO: (depthView == VK_NULL_HANDLE ? 1 : 2),
                    .pAttachments = attachments,
                    .width = static_cast<uint32_t>(swapchain_info_.display_extent.width),
                    .height = static_cast<uint32_t>(swapchain_info_.display_extent.height),
                    .layers = 1,
            };
            fbCreateInfo.attachmentCount = 1; //TODO: 这里需要检查， 是否需要位深 (depthView == VK_NULL_HANDLE ? 1 : 2);

            if (vkCreateFramebuffer(device_info_.logical_device, &fbCreateInfo, nullptr, &swapchain_info_.frame_buffers[i]) != VK_SUCCESS || swapchain_info_.frame_buffers[i] == VK_NULL_HANDLE) {
                LOGE_VVC("Failed to create framebuffer %zu!", i);
            } else {
                succeedCount++;
            }
        }
        if (succeedCount == swapchain_info_.image_count) {
            LOGD_VVC("Frame buffers created: total %d", succeedCount);
            return true;
        } else {
            LOGE_VVC("Failed to create frame buffers: %d/%d", succeedCount, swapchain_info_.image_count);
            return false;
        }
    }

    bool VulkanContext::createVkSyncObjects() {
        VkFenceCreateInfo fenceCreateInfo{
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
        };

        VkResult createFenceResult = vkCreateFence(device_info_.logical_device, &fenceCreateInfo, nullptr, &render_info_.fence);

        if (createFenceResult != VK_SUCCESS || render_info_.fence == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to create fence! %d", createFenceResult);
            return false;
        }

        // We need to create a semaphore to be able to wait, in the main loop, for our
        // framebuffer to be available for us before drawing.
        VkSemaphoreCreateInfo semaphoreCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
        };
        VkResult createSemaphoreResult = vkCreateSemaphore(device_info_.logical_device, &semaphoreCreateInfo, nullptr, &render_info_.semaphore);
        if (createSemaphoreResult != VK_SUCCESS || render_info_.semaphore == VK_NULL_HANDLE) {
            LOGE_VVC("Failed to create semaphore! %d", createSemaphoreResult);
            return false;
        }
        return true;
    }

    void VulkanContext::deleteSwapChain() {
        for (int i = 0; i < swapchain_info_.image_count; i++) {
            vkDestroyFramebuffer(device_info_.logical_device, swapchain_info_.frame_buffers[i], nullptr);
            swapchain_info_.frame_buffers[i] = VK_NULL_HANDLE;
            vkDestroyImageView(device_info_.logical_device, swapchain_info_.image_views[i], nullptr);
            swapchain_info_.image_views[i] = VK_NULL_HANDLE;
        }
        vkDestroySwapchainKHR(device_info_.logical_device, swapchain_info_.swapchain, nullptr);
        swapchain_info_.swapchain = VK_NULL_HANDLE;
    }

    void VulkanContext::deleteGraphicsPipeline()  {
        if (pipeline_info_.cache != VK_NULL_HANDLE) {
            vkDestroyPipelineCache(device_info_.logical_device, pipeline_info_.cache, nullptr);
            pipeline_info_.cache = VK_NULL_HANDLE;
        }
        if (pipeline_info_.layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_info_.logical_device, pipeline_info_.layout, nullptr);
            pipeline_info_.layout = VK_NULL_HANDLE;
        }
        if (pipeline_info_.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_info_.logical_device, pipeline_info_.pipeline, nullptr);
            pipeline_info_.pipeline = VK_NULL_HANDLE;
        }
    }

    void VulkanContext::deleteBuffers()  {
        //vkDestroyBuffer(deviceInfo_.device_, bufferInfos_.vertexBuf_, nullptr);
    }


}
