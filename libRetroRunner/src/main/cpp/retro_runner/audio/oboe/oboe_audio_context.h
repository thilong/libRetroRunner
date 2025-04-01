//
// Created by Aidoo.TK on 2024/11/12.
//

#ifndef _OBOE_AUDIO_CONTEXT_H
#define _OBOE_AUDIO_CONTEXT_H

#include <oboe/Oboe.h>
#include <oboe/FifoBuffer.h>

#include "../audio_context.h"
#include "../resampler/linear_resampler.h"

namespace libRetroRunner {
    class OboeAudioContext : public AudioContext, oboe::AudioStreamDataCallback, oboe::AudioStreamErrorCallback {
    public :
        OboeAudioContext();

        ~OboeAudioContext() override;

        void Init() override;

        void Start() override;

        void Stop() override;

        void Destroy() override;

        void OnAudioSample(int16_t left, int16_t right) override;

        void OnAudioSampleBatch(const int16_t *data, size_t frames) override;

    public:
        oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override;

        void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override;

    private:
        /*
         * 调节输出速度，以防止缓冲区空白或者溢出引起杂音或者爆音
         */
        double calculateDynamicConversionFactor(double dt);

    private:
        const double kp = 0.006;
        const double ki = 0.00002;
        const double maxp = 0.003;
        const double maxi = 0.02;

        double baseConversionFactor = 1.0;
        double framesToSubmit = 0.0;
        double errorIntegral = 0.0;
        double playbackSpeed = 1.0;

        oboe::ManagedStream audioStream;
        std::unique_ptr<oboe::FifoBuffer> audioFifoBuffer;
        std::unique_ptr<uint16_t[]> audioStreamBuffer;
        std::unique_ptr<oboe::LatencyTuner> latencyTuner;


        int bufferSizeInVideoFrame;
        LinearResampler resampler;
    };
}
#endif
