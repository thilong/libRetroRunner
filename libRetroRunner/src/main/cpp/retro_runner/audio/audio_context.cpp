//
// Created by Aidoo.TK on 2024/11/12.
//

#include "audio_context.h"
#include <retro_runner/types/log.h>
#include "empty_audio_context.h"
#include "oboe/oboe_audio_context.h"

namespace libRetroRunner {
    AudioContext::AudioContext() {

    }

    AudioContext::~AudioContext() {

    }

    std::shared_ptr<AudioContext> AudioContext::Create(std::string &driver) {
        if (driver == "android") {
            LOGD("[AUDIO] Create audio context for driver 'android'.");
            return std::make_shared<OboeAudioContext>();
        }
        LOGW("[INPUT] Unsupported audio driver '%s', empty audio context will be used.", driver.c_str());
        return std::make_shared<EmptyAudioContext>();
    }


}
