//
// Created by aidoo.
//

#ifndef LIBRETRORUNNER_RR_VULKAN_PIPELINE_H
#define LIBRETRORUNNER_RR_VULKAN_PIPELINE_H

#include "rr_vulkan.h"

namespace libRetroRunner {
    class VulkanPipeline {
    public:
        VulkanPipeline(VkDevice logicalDevice);

        ~VulkanPipeline();


        bool createRenderPass();

        bool createPipeline(uint32_t width, uint32_t height);

        bool init(uint32_t width, uint32_t height);

        void destroy();

    private:
        bool createShader(void *source, size_t sourceLength, VulkanShaderType shaderType, VkShaderModule *shader);

        bool createDescriptorSetLayout();

    public:
        inline VkFormat getFormat() const { return format_; }

        inline VkRenderPass getRenderPass() const { return renderPass_; }

        inline VkDescriptorSet getDescriptorSet() {
            return descriptorSet_;
        }

        inline VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout_; }

        inline VkPipelineLayout getLayout() const { return layout_; }

        inline VkPipelineCache getCache() const { return cache_; }

        inline VkPipeline getPipeline() const { return pipeline_; }

        inline const std::string &getVertexShaderCode() const { return vertexShaderCode_; }

        inline const std::string &getFragmentShaderCode() const { return fragmentShaderCode_; }

        inline VkShaderModule getVertexShaderModule() const { return vertexShaderModule_; }

        inline VkShaderModule getFragmentShaderModule() const { return fragmentShaderModule_; }

        void setVertexShaderCode(const std::string &code) { vertexShaderCode_ = code; }

        void setFragmentShaderCode(const std::string &code) { fragmentShaderCode_ = code; }

    private:
        VkDevice logicalDevice_;
        VkFormat format_{};

        VkRenderPass renderPass_ = VK_NULL_HANDLE;

        VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;

        VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
        VkPipelineLayout layout_ = VK_NULL_HANDLE;
        VkPipelineCache cache_ = VK_NULL_HANDLE;
        VkPipeline pipeline_ = VK_NULL_HANDLE;

        std::string vertexShaderCode_{};
        std::string fragmentShaderCode_{};
        VkShaderModule vertexShaderModule_ = VK_NULL_HANDLE;
        VkShaderModule fragmentShaderModule_ = VK_NULL_HANDLE;
    };
}
#endif //LIBRETRORUNNER_RR_VULKAN_PIPELINE_H
