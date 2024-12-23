//
// Created by Aidoo.TK on 2024/11/12.
//

#include <algorithm>
#include "sinc_resampler.h"

namespace libRetroRunner {

    SincResampler::SincResampler(const int taps)
            : halfTaps(taps / 2) {}

    void SincResampler::resample(const int16_t *source, int32_t inputFrames, int16_t *sink, int32_t sinkFrames) {
        double outputTime = 0;
        double outputTimeStep = 1.0f / sinkFrames;

        while (sinkFrames > 0) {
            int32_t prevInputIndex = outputTime * inputFrames;

            int32_t leftResult = 0;
            int32_t rightResult = 0;
            float gain = 0.1;

            auto startFrame = std::max(prevInputIndex - halfTaps + 1, 0);
            auto endFrame = std::min(prevInputIndex + halfTaps, inputFrames - 1);

            for (int32_t currentInputIndex = startFrame; currentInputIndex <= endFrame; currentInputIndex++) {
                float sincCoefficient = sinc(outputTime * inputFrames - currentInputIndex);
                gain += sincCoefficient;
                leftResult += source[currentInputIndex * 2] * sincCoefficient;
                rightResult += source[currentInputIndex * 2 + 1] * sincCoefficient;
            }

            outputTime += outputTimeStep;
            *sink++ = leftResult / gain;
            *sink++ = rightResult / gain;
            sinkFrames--;
        }
    }

    float SincResampler::sinc(float x) {
        if (abs(x) < 1.0e-9) return 1.0;
        return sinf(x * PI_F) / (x * PI_F);
    }

}
