//
// Created by aidoo on 2024/11/13.
//

#include "game_context.h"

namespace libRetroRunner {

    GameRuntimeContext::GameRuntimeContext() {
        geometry_max_height_ = 1;
        geometry_max_width_ = 1;
        geometry_height_ = 1;
        geometry_width_ = 1;
        geometry_aspect_ratio_ = 1;
        geometry_rotation_ = 0;
        fps_ = 60;
        sample_rate_ = 44800;
    }

    std::string GameRuntimeContext::GetSaveStateFilePath(int slot) {
        return game_path_ + ".state" + std::to_string(slot);
    }

    GameRuntimeContext::~GameRuntimeContext() = default;
}

