//
// Created by aidoo on 2024/11/4.
//
#include "video_context.h"
#include <memory>

#include "video/video_gl.h"
#include "rr_log.h"
namespace libRetroRunner {

    VideoContext::VideoContext() {};

    VideoContext::~VideoContext() {};

    std::unique_ptr<VideoContext> VideoContext::NewInstance() {
        return std::make_unique<GLVideoContext>();
    }

    void VideoContext::SetTakeScreenshot(std::string &path) {
        dumpPath = path;
        LOGW("set take screenshot path: %s", path.c_str());
    }
}