//
// Created by aidoo on 2024/11/12.
//

#include "audio_context.h"
#include "audio/oboe_audio_context.h"

namespace libRetroRunner {
    AudioContext::AudioContext() {

    }

    AudioContext::~AudioContext() {

    }

    std::unique_ptr<AudioContext> AudioContext::NewInstance() {
        return std::make_unique<OboeAudioContext>();
    }


}
