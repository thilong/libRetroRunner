//
// Created by aidoo on 2024/11/13.
//

#include "core_runtime.h"

namespace libRetroRunner {

    CoreRuntime::CoreRuntime() {
        audio_enabled_ = true;
        video_enabled_ = true;
        max_user_count_ = 4;
        support_no_game_ = false;
        render_context_type_ = -1;
        render_major_version_ = 0;
        render_minor_version_ = 0;
        render_hardware_acceleration_ = false;
        render_depth_ = false;
        render_stencil_ = false;
        render_hw_context_destroy_ = nullptr;
        render_hw_context_reset_ = nullptr;
    }

    CoreRuntime::~CoreRuntime() {

    }
}

