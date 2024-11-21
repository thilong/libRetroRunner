//
// Created by aidoo on 2024/10/31.
//

#include <pthread.h>
#include <unistd.h>
#include <libretro-common/include/libretro.h>

#include "../types/log.h"
#include "../types/app_state.h"
#include "../types/macros.h"

#include "../core/core.h"
#include "environment.h"
#include "app_context.h"
#include "paths.h"

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

    }

    bool libretro_callback_set_environment(unsigned int cmd, void *data) {

        return false;
    }

    void libretro_callback_audio_sample(int16_t left, int16_t right) {

    }

    size_t libretro_callback_audio_sample_batch(const int16_t *data, size_t frames) {
        return frames;
    }

    void libretro_callback_input_poll(void) {

    }

    int16_t libretro_callback_input_state(unsigned int port, unsigned int device, unsigned int index, unsigned int id) {
        int16_t ret = 0;
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

    static std::shared_ptr<AppContext> appInstance(nullptr);

    std::shared_ptr<AppContext> &AppContext::CreateInstance() {
        if (appInstance != nullptr) {
            appInstance->Stop();
        }

        appInstance = std::make_shared<AppContext>();
        return appInstance;
    }

}

/*-----Lifecycle--------------------------------------------------------------*/
namespace libRetroRunner {

    AppContext::AppContext() {
        command_queue_ = std::make_unique<CommandQueue>();
    }

    AppContext::~AppContext() {
        if (appInstance != nullptr && appInstance.get() == this) {
            appInstance = nullptr;
        }
    }

    std::shared_ptr<AppContext> &AppContext::Current() {
        return appInstance;
    }

    void AppContext::ThreadLoop() {
        BIT_SET(state, AppState::kRunning);
        //PrefCounter counter;
        try {
            while (BIT_TEST(state, AppState::kRunning)) {
                processCommand();
                if (!BIT_TEST(state, AppState::kRunning)) break;

                //时间计算, 保持帧率
                // timeThrone.checkFpsAndWait(environment->GetFastForwardFps());
                if (!BIT_TEST(state, AppState::kRunning)) break;

                //if emulate is paused, sleep for 16ms, for 60fps
                if (BIT_TEST(state, AppState::kPaused)) {
                    usleep(16000);
                    continue;
                }

                //run core step when video and content are ready
                if (BIT_TEST(state, AppState::kVideoReady) &&
                    BIT_TEST(state, AppState::kContentReady)) {
                    //TODO: 这里需要做模拟一帧前的准备
                    //video->Prepare();
                    //counter.Reset();

                    core_->retro_run();
                } else {
                    usleep(16000);
                }
            }
        } catch (std::exception &exception) {
            LOGE_APP("emu end with error: %s", exception.what());
        }
        if (BIT_TEST(state, AppState::kContentReady)) {
            core_->retro_unload_game();
            BIT_DELETE(state, AppState::kContentReady);
        }
        if (BIT_TEST(state, AppState::kCoreReady)) {
            core_->retro_deinit();
            BIT_DELETE(state, AppState::kCoreReady);
        }
        BIT_DELETE(state, AppState::kRunning);
        LOGW_APP("emu stopped");
    }

    const std::shared_ptr<class Environment> &AppContext::GetEnvironment() const {
        return environment_;
    }

    const std::shared_ptr<class Paths> &AppContext::GetPaths() const {
        return paths_;
    }

}

/*-----Emulator control--------------------------------------------------------------*/
namespace libRetroRunner {

    void AppContext::Start() {
        if (!BIT_TEST(state, AppState::kPathsReady)) {
            LOGE_APP("Paths are not set yet , can't start emulator.");
            return;
        }
        AddCommand(AppCommands::kLoadCore);
        AddCommand(AppCommands::kLoadContent);
        pthread_t thread;
        int loopRet = pthread_create(&thread, nullptr, libRetroRunner::appThread, this);
        LOGD_APP("emu started, app: %p, thread result: %d", this, loopRet);
    }

    void AppContext::Pause() {

    }

    void AppContext::Resume() {

    }

    void AppContext::Stop() {

    }

    void AppContext::SetPaths(const std::string &rom, const std::string &core, const std::string &system, const std::string &save) {
        paths_ = std::make_shared<Paths>();
        paths_->SetPaths(rom, core, system, save);

        environment_ = std::make_shared<Environment>();

        BIT_SET(state, AppState::kPathsReady);
    }


}

/*-----App commands--------------------------------------------------------------*/
namespace libRetroRunner {
    void AppContext::processCommand() {

    }

    void AppContext::AddCommand(std::shared_ptr<Command> &command) {
        command_queue_->Push(command);
    }

    void AppContext::AddCommand(int command) {
        std::shared_ptr<Command> cmd = std::make_shared<Command>(command);
        command_queue_->Push(cmd);
    }


}

