//
// Created by aidoo on 2024/11/4.
//
#include "video_context.h"
#include <memory>
#include "opengles/video_context_gles.h"
#include "../types/log.h"

namespace libRetroRunner {

    VideoContext::VideoContext() {}

    VideoContext::~VideoContext() {}


    void VideoContext::SetNextScreenshotStorePath(std::string &path) {
        next_screenshot_store_path_ = path;
    }

    std::shared_ptr<VideoContext> VideoContext::Create(std::string &driver) {
        if (driver == "gl") {
            LOGD("[VIDEO] Create OpenGL ES video context for driver 'gl'.");
            return std::make_shared<GLESVideoContext>();
        }
        LOGW("[VIDEO] Unsupported video driver '%s'.", driver.c_str());
        return nullptr;
    }
}