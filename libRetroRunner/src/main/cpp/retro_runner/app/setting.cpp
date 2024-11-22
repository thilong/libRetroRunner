//
// Created by aidoo on 2024/11/13.
//

#include "setting.h"

namespace libRetroRunner {
    static Setting setting;

    Setting::Setting() {

#ifdef ANDROID
        video_driver_ = "gl";
        audio_driver_ = "android";
        input_driver_ = "software";
#endif


    }

    Setting::~Setting() {

    }

    Setting *Setting::Current() {
        return &setting;
    }

}

