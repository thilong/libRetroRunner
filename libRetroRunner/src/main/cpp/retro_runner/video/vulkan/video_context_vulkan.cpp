//
// Created by aidoo on 1/24/2025.
//
#include <vulkan/vulkan.h>

#include "video_context_vulkan.h"
#include "vulkan_api_wrapper.h"

namespace libRetroRunner {

    VulkanVideoContext::VulkanVideoContext() {

    }

    VulkanVideoContext::~VulkanVideoContext() {

    }

    bool VulkanVideoContext::Init() {
        if(!InitVulkan()) return false;
        return true;
    }

    void VulkanVideoContext::Destroy() {

    }

    void VulkanVideoContext::Unload() {

    }

    void VulkanVideoContext::SetSurface(int argc, void **argv) {

    }

    void VulkanVideoContext::SetSurfaceSize(unsigned int width, unsigned int height) {

    }

    void VulkanVideoContext::Prepare() {

    }

    void VulkanVideoContext::OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) {

    }

    void VulkanVideoContext::DrawFrame() {

    }

    unsigned int VulkanVideoContext::GetCurrentFramebuffer() {
        return 0;
    }

    bool VulkanVideoContext::TakeScreenshot(const std::string &path) {
        return false;
    }

}
