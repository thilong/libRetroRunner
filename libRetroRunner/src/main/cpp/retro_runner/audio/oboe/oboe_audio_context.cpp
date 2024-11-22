//
// Created by aidoo on 2024/11/12.
//
#include "oboe_audio_context.h"
#include "../../types/log.h"
#include "../../app/app_context.h"
#include "../../app/environment.h"
#include "../../app/setting.h"

#define LOGD_OBOE(...) LOGD("[OboeAudio] " __VA_ARGS__)
#define LOGW_OBOE(...) LOGW("[OboeAudio] " __VA_ARGS__)
#define LOGE_OBOE(...) LOGE("[OboeAudio] " __VA_ARGS__)
#define LOGI_OBOE(...) LOGI("[OboeAudio] " __VA_ARGS__)


namespace libRetroRunner {
    OboeAudioContext::OboeAudioContext() {

    }

    OboeAudioContext::~OboeAudioContext() {
        audioStream->requestStop();
    }

    void OboeAudioContext::Start() {
        audioStream->requestStart();
    }

    void OboeAudioContext::Stop() {
        audioStream->requestStop();
    }

    void OboeAudioContext::OnAudioSample(int16_t left, int16_t right) {
        audioFifoBuffer->write(&left, 1);
        audioFifoBuffer->write(&right, 1);
    }

    void OboeAudioContext::OnAudioSampleBatch(const int16_t *data, size_t frames) {
        audioFifoBuffer->write(data, frames * 2);
    }

    void OboeAudioContext::Init() {
        auto setting = Setting::Current();
        auto environment = AppContext::Current()->GetEnvironment();


        bool preferLowLatency = setting->UseLowLatency();
        bool useLowLatency = false;
        LOGD_OBOE("prefer low latency: %d", preferLowLatency);
        if (oboe::AudioStreamBuilder::isAAudioRecommended() && preferLowLatency) {
            bufferSizeInVideoFrame = 4;
            useLowLatency = true;
        } else {
            bufferSizeInVideoFrame = 8;
            useLowLatency = false;
        }

        double maxLatency = std::max(bufferSizeInVideoFrame / 60.0 * 1000, 32.0);
        double sampleRateDivisor = 500.0 / maxLatency;
        int audioBufferSize = ((int) (environment->GetSampleRate() / sampleRateDivisor)) / 2 * 2;

        oboe::AudioStreamBuilder streamBuilder;
        streamBuilder.setFormat(oboe::AudioFormat::I16);
        streamBuilder.setChannelCount(2);
        streamBuilder.setDirection(oboe::Direction::Output);
        streamBuilder.setDataCallback(this);
        streamBuilder.setErrorCallback(this);

        streamBuilder.setSampleRate(environment->GetSampleRate());
        streamBuilder.setPerformanceMode(oboe::PerformanceMode::LowLatency);

        oboe::Result result = streamBuilder.openManagedStream(audioStream);
        if (result != oboe::Result::OK) {
            LOGE_OBOE("Failed to open audio stream: %s", oboe::convertToText(result));
            audioStream = nullptr;
            latencyTuner = nullptr;
            return;
        }

        double latency = environment->GetSampleRate() / (double) environment->GetSampleRate();  //延迟计算
        audioFifoBuffer = std::make_unique<oboe::FifoBuffer>(2, audioBufferSize);
        audioStreamBuffer = std::unique_ptr<uint16_t[]>(new uint16_t[audioBufferSize]);
        latencyTuner = std::make_unique<oboe::LatencyTuner>(*audioStream);

    }

    oboe::DataCallbackResult OboeAudioContext::onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) {
        double dynamicBufferFactor = calculateDynamicConversionFactor(0.001 * numFrames);
        double finalConversionFactor = baseConversionFactor * dynamicBufferFactor * playbackSpeed;

        // 使用低延迟时，numFrames 非常低（~100），动态缓冲区缩放不适用于舍入。
        // 通过跟踪“小数”帧，我们可以将错误保持在较小水平。
        framesToSubmit += numFrames * finalConversionFactor;
        int32_t currentFramesToSubmit = std::round(framesToSubmit);
        framesToSubmit -= currentFramesToSubmit;

        //读出音频数据
        audioFifoBuffer->readNow(audioStreamBuffer.get(), currentFramesToSubmit * 2);

        //重采样后输出
        auto outputArray = reinterpret_cast<int16_t *>(audioData);
        resampler.resample(reinterpret_cast<const int16_t *>(audioStreamBuffer.get()), currentFramesToSubmit, outputArray, numFrames);

        latencyTuner->tune();

        return oboe::DataCallbackResult::Continue;
    }

    void OboeAudioContext::onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) {
        AudioStreamErrorCallback::onErrorAfterClose(oboeStream, error);
    }

    double OboeAudioContext::calculateDynamicConversionFactor(double dt) {
        double framesCapacityInBuffer = audioFifoBuffer->getBufferCapacityInFrames();
        double framesAvailableInBuffer = audioFifoBuffer->getFullFramesAvailable();

        // 误差用与一半缓冲区利用率的标准化距离表示。范围 [-1.0, 1.0]
        double errorMeasure = (framesCapacityInBuffer - 2.0f * framesAvailableInBuffer) / framesCapacityInBuffer;

        errorIntegral += errorMeasure * dt;

        // 人耳在 1000-2000 Hz 的声音频率内内解析度约为 3.6 Hz
        // 由于它会不断变化，所以我们应尽量将其保持在非常低的值。
        double proportionalAdjustment = std::clamp(kp * errorMeasure, -maxp, maxp);

        // 高低Ki的值 ，即使超过了耳朵的阈值，也会更安全
        // 我们需要测试这个值，让它收敛的速度足够慢，以致于无法察觉或听到任何杂音
        double integralAdjustment = std::clamp(ki * errorIntegral, -maxi, maxi);

        double finalAdjustment = proportionalAdjustment + integralAdjustment;

        return 1.0 - (finalAdjustment);
    }


}
