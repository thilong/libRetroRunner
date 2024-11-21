//
// Created by aidoo on 2024/10/31.
//

#include <pthread.h>
#include <unistd.h>
#include <libretro-common/include/libretro.h>

#ifdef ANDROID

#include <jni.h>

#endif

#define LOGD_APP(...) LOGD("[APP] " __VA_ARGS__)
#define LOGW_APP(...) LOGW("[APP] " __VA_ARGS__)
#define LOGE_APP(...) LOGE("[APP] " __VA_ARGS__)
#define LOGI_APP(...) LOGI("[APP] " __VA_ARGS__)

/*-----RETRO CALLBACKS--------------------------------------------------------------*/
namespace libRetroRunner {
    void libretro_callback_hw_video_refresh(const void *data, unsigned int width, unsigned int height, size_t pitch) {
        AppContext *appContext = AppContext::Current();
        if (appContext) {
            VideoContext *video = appContext->GetVideo();
            if (video) video->OnNewFrame(data, width, height, pitch);
        }
    }

    bool libretro_callback_set_environment(unsigned int cmd, void *data) {
        AppContext *appContext = AppContext::Current();
        if (appContext) {
            return appContext->GetEnvironment()->HandleCoreCallback(cmd, data);
        }
        return false;
    }

    void libretro_callback_audio_sample(int16_t left, int16_t right) {
        AppContext *appContext = AppContext::Current();
        if (appContext) {
            auto audio = appContext->GetAudio();
            if (audio != nullptr) audio->OnAudioSample(left, right);
        }
    }

    size_t libretro_callback_audio_sample_batch(const int16_t *data, size_t frames) {
        AppContext *appContext = AppContext::Current();
        if (appContext) {
            auto audio = appContext->GetAudio();
            if (audio != nullptr) audio->OnAudioSampleBatch(data, frames);
        }
        return frames;
    }

    void libretro_callback_input_poll(void) {
        auto appContext = AppContext::Current();
        if (appContext) {
            auto input = appContext->GetInput();
            if (input != nullptr) input->Poll();
        }
    }

    int16_t libretro_callback_input_state(unsigned int port, unsigned int device, unsigned int index, unsigned int id) {
        int16_t ret = 0;
        auto appContext = AppContext::Current();
        if (appContext) {
            auto input = appContext->GetInput();
            if (input != nullptr)
                ret = input->State(port, device, index, id);
        }
        return ret;
    }
}


namespace libRetroRunner {

    extern "C" JavaVM *gVm;

    static void *appThread(void *args) {
        ((AppContext *) args)->ThreadLoop();
        LOGW_APP("emu thread exit.");
        return nullptr;
    }

    std::unique_ptr<AppContext> AppContext::instance = nullptr;

    AppContext *AppContext::CreateInstance() {
        instance = std::make_unique<AppContext>();
        return instance.get();
    }

}

/*-----生命周期相关--------------------------------------------------------------*/
namespace libRetroRunner {

    AppContext::AppContext() {
    }

    AppContext::~AppContext() {
        if (instance != nullptr && instance.get() == this) {
            instance = nullptr;
        }
        BIT_DELETE(state, AppState::kRunning);

        core_path = "";
        system_path = "";
        rom_path = "";

        input = nullptr;
        core = nullptr;
        video = nullptr;
        audio == nullptr;
        cheatManager = nullptr;
        environment = nullptr;
    }

    void AppContext::ThreadLoop() {
        BIT_SET(state, AppState::kRunning);
        JNIEnv *env;
        PrefCounter counter;
        try {
            while (BIT_TEST(state, AppState::kRunning)) {
                //先处理命令
                processCommand();
                if (!BIT_TEST(state, AppState::kRunning)) break;

                //时间计算, 保持帧率
                timeThrone.checkFpsAndWait(environment->GetFastForwardFps());

                if (!BIT_TEST(state, AppState::kRunning)) break;
                //如果模拟处于暂停状态，则等待16ms, 60fps对应1帧16ms
                if (BIT_TEST(state, AppState::kPaused)) {
                    //sleep for 16ms, for 60fps
                    usleep(16000);
                    continue;
                }
                //只有视频和内容都准备好了才能运行
                if (BIT_TEST(state, AppState::kVideoReady) &&
                    BIT_TEST(state, AppState::kContentReady)) {
                    gVm->AttachCurrentThread(&env, nullptr);
                    //TODO: 这里需要做模拟一帧前的准备
                    //video->Prepare();
                    counter.Reset();
                    core->retro_run();
                    //LOGW("core run: %lld", counter.ElapsedNano());
                    gVm->DetachCurrentThread();
                } else {
                    //如果视频输出环境没有OK，或者内容没有加载，则等待1帧
                    usleep(16000);
                }
            }
        } catch (std::exception &exception) {
            LOGE_APP("emu end with error: %s", exception.what());
        }
        if (BIT_TEST(state, AppState::kContentReady)) {
            core->retro_unload_game();
            BIT_DELETE(state, AppState::kContentReady);
        }
        if (BIT_TEST(state, AppState::kCoreReady)) {
            core->retro_deinit();
            BIT_DELETE(state, AppState::kCoreReady);
        }
        BIT_DELETE(state, AppState::kRunning);
        LOGW_APP("emu stopped");
    }

}


