//
// Created by aidoo.
//
#include "rr_vulkan_pipeline.h"

#include "shader_code_frag.h"
#include "shader_code_vert.h"

namespace libRetroRunner {
    VulkanPipeline::VulkanPipeline(VkDevice logicalDevice) {
        logicalDevice_ = logicalDevice;
        format_ = VK_FORMAT_R8G8B8A8_UNORM;
    }

    VulkanPipeline::~VulkanPipeline() {
        destroy();
    }

    bool VulkanPipeline::createRenderPass() {
        VkAttachmentDescription attachmentDescriptions{
                .format = format_,
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

        VkResult createRenderPassResult = vkCreateRenderPass(logicalDevice_, &renderPassCreateInfo, nullptr, &renderPass_);

        if (createRenderPassResult || renderPass_ == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create render pass: %d\n", createRenderPassResult);
            return false;
        } else {
            LOGD_VC("Render pass created: %p\n", renderPass_);
        }
        return true;
    }

    bool VulkanPipeline::createPipeline(uint32_t width, uint32_t height) {
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .renderPass = renderPass_
        };
        //视图与裁剪
        VkViewport viewport{
                .width = (float) width,
                .height = (float) height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f
        };
        VkRect2D scissor{
                .offset = {0, 0},
                .extent = {width, height}
        };
        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .pViewports = &viewport,
                .scissorCount = 1,
                .pScissors = &scissor
        };
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;

        //图元组装方式
        VkPipelineInputAssemblyStateCreateInfo assemblyStateCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,                       //每相邻的三个点组成一个三角形
                .primitiveRestartEnable = VK_FALSE                                      //是否使用特定的值来表示图元结束
        };
        pipelineCreateInfo.pInputAssemblyState = &assemblyStateCreateInfo;

        //着色器
        /*
        if (!createShader(vertexShaderCode_.data(), vertexShaderCode_.length(), VulkanShaderType::SHADER_VERTEX, &vertexShaderModule_)) {
            LOGE_VC("failed to create vertex shader.");
        }
        if (!createShader(fragmentShaderCode_.data(), fragmentShaderCode_.length(), VulkanShaderType::SHADER_FRAGMENT, &fragmentShaderModule_)) {
            LOGE_VC("failed to create fragment shader.");
        }*/
        if (!createShader((void *) shader_vertex_code, shader_vertex_code_size, VulkanShaderType::SHADER_VERTEX, &vertexShaderModule_)) {
            LOGE_VC("failed to create vertex shader.");
        }
        if (!createShader((void *) shader_fragment_code, shader_fragment_code_size, VulkanShaderType::SHADER_FRAGMENT, &fragmentShaderModule_)) {
            LOGE_VC("failed to create fragment shader.");
        }

        //Vertex binding
        VkVertexInputBindingDescription vertexInputBindingDescription{
                .binding = 0,
                .stride = 6 * sizeof(float),                                            //每个顶点的字节数
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

        //Fragment binding (pipeline layout)
        if (!createDescriptorSetLayout())
            return false;

        VkPipelineLayoutCreateInfo layoutCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 1,
                .pSetLayouts = descriptorSetLayout_ ? &descriptorSetLayout_ : nullptr
        };
        VkResult layoutCreateResult = vkCreatePipelineLayout(logicalDevice_, &layoutCreateInfo, nullptr, &layout_);
        if (layoutCreateResult != VK_SUCCESS || layout_ == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create pipeline layout: %d", layoutCreateResult);
            return false;
        }
        pipelineCreateInfo.layout = layout_;

        //shader stage
        VkPipelineShaderStageCreateInfo stageCreateInfo[2]{
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_VERTEX_BIT,
                        .module = vertexShaderModule_,
                        .pName = "main"
                },
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                        .module = fragmentShaderModule_,
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
        VkResult pipelineCacheCreateResult = vkCreatePipelineCache(logicalDevice_, &pipelineCacheCreateInfo, nullptr, &cache_);
        if (pipelineCacheCreateResult != VK_SUCCESS || cache_ == VK_NULL_HANDLE) {
            LOGE_VC("pipeline cache create failed: %d", pipelineCacheCreateResult);
        } else {
            LOGD_VC("pipeline cache:\t%p", cache_);
        }

        VkResult pipelineCreateResult = vkCreateGraphicsPipelines(logicalDevice_, cache_, 1, &pipelineCreateInfo, nullptr, &pipeline_);
        if (pipelineCreateResult != VK_SUCCESS || pipeline_ == VK_NULL_HANDLE) {
            LOGE_VC("pipeline create failed: %d\n", pipelineCreateResult);
        } else {
            LOGD_VC("pipeline:\t\t%p", pipeline_);
        }
        return true;
    }

    bool VulkanPipeline::init(uint32_t width, uint32_t height) {
        if (!createRenderPass()) {
            destroy();
            return false;
        }
        if (!createPipeline(width, height)) {
            destroy();
            return false;
        }
        return true;
    }

    void VulkanPipeline::destroy() {
        if (vertexShaderModule_) {
            vkDestroyShaderModule(logicalDevice_, vertexShaderModule_, nullptr);
            vertexShaderModule_ = VK_NULL_HANDLE;
        }
        if (fragmentShaderModule_) {
            vkDestroyShaderModule(logicalDevice_, fragmentShaderModule_, nullptr);
            fragmentShaderModule_ = VK_NULL_HANDLE;
        }
        if (pipeline_) {
            vkDestroyPipeline(logicalDevice_, pipeline_, nullptr);
            pipeline_ = VK_NULL_HANDLE;
        }
        if (cache_) {
            vkDestroyPipelineCache(logicalDevice_, cache_, nullptr);
            cache_ = VK_NULL_HANDLE;
        }
        if (!descriptorSet_) {
            vkFreeDescriptorSets(logicalDevice_, descriptorPool_, 1, &descriptorSet_);
            descriptorSet_ = nullptr;
        }
        if (descriptorPool_) {

            vkDestroyDescriptorPool(logicalDevice_, descriptorPool_, nullptr);
            descriptorPool_ = VK_NULL_HANDLE;
        }
        if (descriptorSetLayout_) {
            vkDestroyDescriptorSetLayout(logicalDevice_, descriptorSetLayout_, nullptr);
            descriptorSetLayout_ = VK_NULL_HANDLE;
        }
        if (layout_) {
            vkDestroyPipelineLayout(logicalDevice_, layout_, nullptr);
            layout_ = VK_NULL_HANDLE;
        }
        if (renderPass_) {
            vkDestroyRenderPass(logicalDevice_, renderPass_, nullptr);
            renderPass_ = VK_NULL_HANDLE;
        }

    }

    bool VulkanPipeline::createShader(void *source, size_t sourceLength, VulkanShaderType shaderType, VkShaderModule *shader) {
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

    bool VulkanPipeline::createDescriptorSetLayout() {
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

        VkResult descSetLayoutCreateResult = vkCreateDescriptorSetLayout(logicalDevice_, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout_);
        if (descSetLayoutCreateResult != VK_SUCCESS || descriptorSetLayout_ == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create descriptor set layout: %d", descSetLayoutCreateResult);
            return false;
        }
        LOGD_VC("Descriptor set layout created: %p", descriptorSetLayout_);

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
        VkResult poolCreateResult = vkCreateDescriptorPool(logicalDevice_, &poolCreateInfo, nullptr, &descriptorPool_);
        if (poolCreateResult != VK_SUCCESS || descriptorPool_ == VK_NULL_HANDLE) {
            LOGE_VC("Failed to create descriptor pool: %d", poolCreateResult);
            return false;
        }
        LOGD_VC("Descriptor pool created: %p", descriptorPool_);

        VkDescriptorSetAllocateInfo allocateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = descriptorPool_,
                .descriptorSetCount = 1,
                .pSetLayouts = &descriptorSetLayout_
        };
        VkResult allocateResult = vkAllocateDescriptorSets(logicalDevice_, &allocateInfo, &descriptorSet_);
        if (allocateResult != VK_SUCCESS) {
            LOGE_VC("Failed to allocate descriptor set: %d", allocateResult);
            return false;
        }
        LOGD_VC("Descriptor set allocated: %p", descriptorSet_);

        return true;
    }
}