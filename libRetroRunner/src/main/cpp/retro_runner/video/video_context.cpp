//
// Created by Aidoo.TK on 2024/11/4.
//
#include "video_context.h"
#include <memory>
#include "opengles/video_context_gles.h"
#include "vulkan/video_context_vulkan.h"
#include "../types/log.h"

namespace libRetroRunner {

    VideoContext::VideoContext() {
        enabled_ = false;
    }

    VideoContext::~VideoContext() {}


    void VideoContext::SetNextScreenshotStorePath(std::string &path) {
        next_screenshot_store_path_ = path;
    }

    std::shared_ptr<VideoContext> VideoContext::Create(std::string &driver) {
        if (driver == "gl") {
            LOGD("[VIDEO] Create OpenGL ES video context for driver 'gl'.");
            return std::make_shared<GLESVideoContext>();
        } else if(driver == "vulkan"){
            LOGD("[VIDEO] Create Vulkan video context for driver 'vulkan'.");
            return std::make_shared<VulkanVideoContext>();
        }
        LOGW("[VIDEO] Unsupported video driver '%s'.", driver.c_str());
        return nullptr;
    }

    void VideoContext::SetEnabled(bool flag) {
        enabled_ = flag;
    }

    void VideoContext::SetGameContext(std::shared_ptr<GameRuntimeContext> &ctx) {
        game_runtime_ctx_ = ctx;
    }
}