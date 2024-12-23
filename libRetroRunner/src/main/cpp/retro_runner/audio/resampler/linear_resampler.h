//
// Created by Aidoo.TK on 2024/11/12.
//

#ifndef _LINEAR_RESAMPLER_H
#define _LINEAR_RESAMPLER_H

#include "resampler.h"

namespace libRetroRunner {

    class LinearResampler : public Resampler {
    public:
        void resample(const int16_t *source, int32_t inputFrames, int16_t *sink, int32_t sinkFrames) override;

        LinearResampler() = default;

        virtual ~LinearResampler() = default;
    };

}

#endif
