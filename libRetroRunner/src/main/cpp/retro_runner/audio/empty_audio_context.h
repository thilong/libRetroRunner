//
// Created by Aidoo.TK on 11/22/2024.
//

#ifndef _EMPTY_AUDIO_CONTEXT_H
#define _EMPTY_AUDIO_CONTEXT_H
#include "audio_context.h"

namespace libRetroRunner {
    class EmptyAudioContext : public AudioContext {
    public:
        EmptyAudioContext();

        ~EmptyAudioContext();

         void Init() override;

         void Start() override;

         void Stop() override;

         void OnAudioSample(int16_t left, int16_t right) override;

         void OnAudioSampleBatch(const int16_t *data, size_t frames) override;

    };

}

#endif
