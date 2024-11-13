//
// Created by aidoo on 2024/11/12.
//

#ifndef _RESAMPLER_H
#define _RESAMPLER_H

#include <cstdint>

namespace libRetroRunner {

    class Resampler {
    public:
        virtual void resample(const int16_t *source, int32_t inputFrames, int16_t *sink, int32_t sinkFrames) = 0;

        virtual ~Resampler() = default;
    };
}

#endif
