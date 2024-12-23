//
// Created by Aidoo.TK on 2024/11/12.
//


#ifndef _SINC_RESAMPLER_H
#define _SINC_RESAMPLER_H

#include "resampler.h"

namespace libRetroRunner {

class SincResampler : public Resampler {
public:
    void resample(const int16_t *source, int32_t inputFrames, int16_t *sink, int32_t sinkFrames) override;
    SincResampler(const int taps);
    ~SincResampler() = default;

private:
    static float sinc(float x);

private:
    static constexpr float PI_F = 3.14159265358979f;
    int halfTaps;
};

}

#endif
