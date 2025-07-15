//
// Created by aidoo on 1/24/2025.
//
#include <vector>
#include <queue>
#include <memory>

#include "../video_context.h"

#include <libretro-common/include/libretro_vulkan.h>
#include <libretro-common/include/rthreads/rthreads.h>

#ifndef LIBRETRORUNNER_VIDEO_CONTEXT_VULKAN_H
#define LIBRETRORUNNER_VIDEO_CONTEXT_VULKAN_H

class VulkanSamplingTexture;

class VulkanRWBuffer;

namespace libRetroRunner {
    enum VulkanShaderType {
        SHADER_VERTEX, SHADER_FRAGMENT
    };

    struct RRVulkanColor {
        float red, green, blue, alpha;
    };

    struct RRVulkanVertex {
        float x, y;
        float texX, texY;
        RRVulkanColor color;
    };

    struct RRVulkanImage {
        VkImage image;
        VkImageView imageView;
        VkFramebuffer frameBuffer;
        VkDeviceMemory memory;
    };

    struct RRVulkanTexture {
        VkDeviceSize memorySize = 0;

        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        size_t stride;  //size of one line pixel in bytes
        size_t size;    //size of the whole texture in bytes
        unsigned width, height;

        VkImageLayout layout;
        VkFormat format;
    };

    struct RRVulkanBuffer {
        VkDeviceSize size = 0;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    enum RRVulkanSurfaceState {
        RRVULKAN_SURFACE_STATE_INVALID = 0,
        RRVULKAN_SURFACE_STATE_VALID = 1 << 0,
        RRVULKAN_SURFACE_STATE_CAPABILITY_SET = 1 << 1,
        RRVULKAN_SURFACE_STATE_CORE_CONTEXT_LOADED = 1 << 2
    };

    struct RRVulkanSurfaceContext {
        int flags = RRVULKAN_SURFACE_STATE_INVALID;

        long surfaceId = 0;

        VkSurfaceKHR surface = VK_NULL_HANDLE;

        uint32_t imageCount = 2;   //飞行帧的数量

        VkExtent2D extent{0, 0};
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    };

    struct RRVulkanFrameContext {
        struct RRVulkanTexture texture{};
        struct RRVulkanBuffer stagingBuffer{};


        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        VkFence fence = VK_NULL_HANDLE;
        VkSemaphore renderSemaphore = VK_NULL_HANDLE;
        VkSemaphore imageAcquireSemaphore = VK_NULL_HANDLE;
    };

    enum RRVulkanRenderState {
        RRVULKAN_RENDER_STATE_INVALID = 0,
        RRVULKAN_RENDER_STATE_RENDER_PASS_VALID = 1 << 0,
        RRVULKAN_RENDER_STATE_SHADERS_VALID = 1 << 1,
        RRVULKAN_RENDER_STATE_DESCRIPTOR_SET_LAYOUT_VALID = 1 << 2,
        RRVULKAN_RENDER_STATE_PIPELINE_LAYOUT_VALID = 1 << 3,
        RRVULKAN_RENDER_STATE_PIPELINE_CACHE_VALID = 1 << 4,
        RRVULKAN_RENDER_STATE_PIPELINE_VALID = 1 << 5,
        RRVULKAN_RENDER_STATE_DESCRIPTOR_POOL_VALID = 1 << 6,
    };

    struct RRVulkanRenderContext {
        int flag = RRVULKAN_RENDER_STATE_INVALID;

        VkRenderPass renderPass = VK_NULL_HANDLE;

        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineCache pipelineCache = VK_NULL_HANDLE;

        char *vertexShaderCode = nullptr;
        char *fragmentShaderCode = nullptr;
        VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
        VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;


        VkCommandPool commandPool = VK_NULL_HANDLE;

        std::vector<RRVulkanFrameContext> frames{};

        uint32_t current_frame = 0;
    };

    enum RRVulkanSwapChainState {
        RRVULKAN_SWAPCHAIN_STATE_INVALID = 0,
        RRVULKAN_SWAPCHAIN_STATE_SWAPCHAIN_VALID = 1 << 0,
        RRVULKAN_SWAPCHAIN_STATE_IMAGES_VALID = 1 << 1,
        RRVULKAN_SWAPCHAIN_STATE_SIGNAL_OBJ_VALID = 1 << 2,
        RRVULKAN_SWAPCHAIN_STATE_NEED_RECREATE = 1 << 3,
    };

    struct RRVulkanSwapchainContext {
        int flag = RRVULKAN_SWAPCHAIN_STATE_INVALID;

        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;

        std::vector<VkImage> images{};
        std::vector<VkImageView> imageViews{};

        std::vector<VkFramebuffer> frameBuffers{};

    };

    class VulkanVideoContext : public VideoContext {
    public:
        VulkanVideoContext();

        ~VulkanVideoContext() override;

        bool Load() override;

        void Unload() override;

        void SetWindowPaused() override;

        void UpdateVideoSize(unsigned width, unsigned height) override;

        void Destroy() override;

        void Prepare() override;

        void OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) override;

        void DrawFrame() override;

        bool TakeScreenshot(const std::string &path) override;

        bool getRetroHardwareRenderInterface(void **) override;


    public:
        VkInstance retro_vulkan_create_instance_wrapper(const VkInstanceCreateInfo *create_info) const;

        VkDevice retro_vulkan_create_device_wrapper(VkPhysicalDevice gpu, const VkDeviceCreateInfo *create_info);

        void retro_vulkan_set_image_t_impl(const struct retro_vulkan_image *image, uint32_t num_semaphores, const VkSemaphore *semaphores, uint32_t src_queue_family);

        uint32_t retro_vulkan_get_sync_index_t_impl() const;

        uint32_t retro_vulkan_get_sync_index_mask_t_impl() const;

        void retro_vulkan_set_command_buffers_t_impl(uint32_t num_cmd, const VkCommandBuffer *cmd) const;

        void retro_vulkan_wait_sync_index_t_impl();

        void retro_vulkan_lock_queue_t_impl();

        void retro_vulkan_unlock_queue_t_impl();

        void retro_vulkan_set_signal_semaphore_t_impl(VkSemaphore semaphore);

    private:
        void recordCommandBufferForSoftwareRender(void *pCommandBuffer, uint32_t imageIndex, VkDescriptorSet frameDescriptorSet);

        void vulkanCommitFrame();

        bool vulkanCreateInstanceIfNeeded();

        bool vulkanCreateSurfaceIfNeeded();

        bool vulkanChooseGPU();

        bool vulkanCreateDeviceIfNeeded();

        bool vulkanGetSurfaceCapabilitiesIfNeeded();

        bool vulkanCreateCommandPoolIfNeeded();

        bool vulkanCreateSwapchainIfNeeded();

        bool vulkanCreateRenderPassIfNeeded();

        bool vulkanCreateDescriptorSetLayoutIfNeeded();

        bool vulkanCreateGraphicsPipelineIfNeeded();

        bool vulkanCreateDescriptorPoolIfNeeded();

        bool vulkanCreateFrameResourcesIfNeeded();

        bool vulkanCreateSwapchainResourcesIfNeeded();

        void vulkanCreateDrawingResourceIfNeeded(uint32_t width, uint32_t height);

        void vulkanRecreateSwapchainIfNeeded();

        bool createShader(void *source, size_t sourceLength, VulkanShaderType shaderType, VkShaderModule *shader);

        void fillFrameTexture(const void *data, unsigned int width, unsigned int height, size_t pitch);
    public:

        void vulkanClearRenderContextIfNeeded();

        bool vulkanClearSwapchainIfNeeded();

        bool vulkanClearSwapchainResourcesIfNeeded();

        bool vulkanClearFrameResourcesIfNeeded();


    private:
        const retro_hw_render_context_negotiation_interface_vulkan *getNegotiationInterface();

    private:
        void *window_;

        bool is_vulkan_debug_;
        int core_pixel_format_;

        uint32_t screen_width_;
        uint32_t screen_height_;

        retro_hw_render_interface_vulkan *retro_render_interface_;


        bool vulkanIsReady_{};

        VkInstance instance_;
        VkPhysicalDevice physicalDevice_;
        VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures{};
        VkPhysicalDeviceProperties physicalDeviceProperties_{};
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties_{};

        VkDevice logicalDevice_;

        VkQueue presentationQueue_;
        uint32_t presentationQueueFamilyIndex_ = 0;
        pthread_mutex_t *queue_lock = nullptr;

        uint32_t framesInFlight_ = 2;


        RRVulkanSurfaceContext surfaceContext_{};
        RRVulkanRenderContext renderContext_{};
        RRVulkanSwapchainContext swapchainContext_{};


        retro_vulkan_destroy_device_t destroyDeviceImpl_ = nullptr;

        const retro_vulkan_image *negotiationImage_ = nullptr;
        uint32_t negotiationSemaphoreCount_ = 0;
        VkSemaphore *negotiationSemaphores_ = nullptr;
        VkPipelineStageFlags *negotiationWaitStages_= nullptr;
        uint32_t negotiationQueueFamily_ = 0;
        VkSemaphore negotiationSemaphore_ =VK_NULL_HANDLE ;
        std::vector<VkCommandBuffer> negotiationCommandBuffers_{};

        VulkanRWBuffer *vertexBuffer_ = nullptr;
        VkSampler sampler_ = VK_NULL_HANDLE;
        uint64_t frameCount_ = 0;

        bool videoContentNeedUpdate_ = false;

        std::vector<unsigned char> convertingBuffer;
    };
}


#endif //LIBRETRORUNNER_VIDEO_CONTEXT_VULKAN_H
