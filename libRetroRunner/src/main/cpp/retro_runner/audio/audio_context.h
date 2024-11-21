//
// Created by aidoo on 2024/11/12.
//

#ifndef _AUDIO_CONTEXT_H
#define _AUDIO_CONTEXT_H

#include "libretro-common/include/libretro.h"
#include <memory>

namespace libRetroRunner {
    class AudioContext {
    public:
        AudioContext();

        virtual ~AudioContext();

        static std::unique_ptr<AudioContext> NewInstance();

        virtual void Init() = 0;

        virtual void Start() = 0;

        virtual void Stop() = 0;

        virtual void OnAudioSample(int16_t left, int16_t right) = 0;

        virtual void OnAudioSampleBatch(const int16_t *data, size_t frames) = 0;
    };
}
#endif
