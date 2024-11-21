//
// Created by aidoo on 2024/11/12.
//

#include <cmath>
#include <algorithm>
#include "linearresampler.h"

namespace libRetroRunner {

    void LinearResampler::resample(const int16_t *source, int32_t inputFrames, int16_t *sink, int32_t sinkFrames) {
        double outputTime = 0;
        double outputTimeStep = 1.0 / sinkFrames;

        double floatingPart, integerPart;

        while (sinkFrames > 0) {
            floatingPart = std::modf(outputTime * inputFrames, &integerPart);

            int32_t floorFrame = integerPart;
            int32_t ceilFrame = std::min(floorFrame + 1, inputFrames - 1);

            *sink++ = source[ceilFrame * 2] * (floatingPart) + source[floorFrame * 2] * (1.0 - floatingPart);
            *sink++ = source[ceilFrame * 2 + 1] * (floatingPart) + source[floorFrame * 2 + 1] * (1.0 - floatingPart);
            outputTime += outputTimeStep;
            sinkFrames--;
        }
    }

}
