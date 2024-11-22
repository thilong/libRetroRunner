//
// Created by aidoo on 2024/11/13.
//

#include "setting.h"

namespace libRetroRunner {
    static Setting setting;

    Setting::Setting() {
#ifdef ANDROID
        video_driver_ = "gl";
#endif
    }

    Setting::~Setting() {

    }

    Setting *Setting::Current() {
        return &setting;
    }

}

