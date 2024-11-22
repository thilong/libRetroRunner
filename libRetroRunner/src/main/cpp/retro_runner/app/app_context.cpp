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
#include "../utils/utils.h"
#include "../video/video_context.h"
#include "setting.h"
#include "../input/input_context.h"
#include "../audio/audio_context.h"

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
        auto appContext = AppContext::Current();
        if (appContext) {
            auto video = appContext->GetVideo();
            if (video) video->OnNewFrame(data, width, height, pitch);
        }
    }

    bool libretro_callback_set_environment(unsigned int cmd, void *data) {
        auto appContext = AppContext::Current();
        if (appContext) {
            return appContext->GetEnvironment()->HandleCoreCallback(cmd, data);
        }
        return false;
    }

    void libretro_callback_audio_sample(int16_t left, int16_t right) {
        auto appContext = AppContext::Current();
        if (appContext) {
            auto audio = appContext->GetAudio();
            if (audio != nullptr) audio->OnAudioSample(left, right);
        }
    }

    size_t libretro_callback_audio_sample_batch(const int16_t *data, size_t frames) {
        auto appContext = AppContext::Current();
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
        auto appContext = AppContext::Current();
        if (appContext) {
            auto input = appContext->GetInput();
            if (input != nullptr)
                return input->State(port, device, index, id);
        }
        return 0;
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

                    //avoid emulator run too fast
                    fps_time_throne_.checkFpsAndWait(environment_->GetFastForwardFps());

                    video_->Prepare();
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

    const std::shared_ptr<class VideoContext> &AppContext::GetVideo() const {
        return video_;
    }

    const std::shared_ptr<class InputContext> &AppContext::GetInput() const {
        return input_;
    }

    const std::shared_ptr<class AudioContext> &AppContext::GetAudio() const {
        return audio_;
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
        AddCommand(AppCommands::kPauseGame);
        AddCommand(AppCommands::kDisableAudio);
    }

    void AppContext::Resume() {
        BIT_DELETE(state, AppState::kPaused);
        AddCommand(AppCommands::kEnableAudio);
    }

    void AppContext::Reset() {
        if (BIT_TEST(state, kContentReady)) {
            AddCommand(AppCommands::kResetGame);
        }
    }

    void AppContext::Stop() {
        AddCommand(AppCommands::kSaveSRAM);
        AddCommand(AppCommands::kStopGame);
    }

    void AppContext::SetPaths(const std::string &rom, const std::string &core, const std::string &system, const std::string &save) {
        paths_ = std::make_shared<Paths>();
        paths_->SetPaths(rom, core, system, save);

        environment_ = std::make_shared<Environment>();

        BIT_SET(state, AppState::kPathsReady);
    }

    void AppContext::SetVideoSurface(int argc, void **argv) {
        if (argc < 1) {
            LOGE_APP("SetVideoSurface: invalid arguments");
            return;
        }
        if (argv[1] == 0) {
            BIT_DELETE(state, AppState::kVideoReady);
            AddCommand(AppCommands::kUnloadVideo);
            return;
        }
        std::string driver = Setting::Current()->GetVideoDriver();
        video_ = VideoContext::Create(driver);
        if (video_) {
            video_->SetSurface(argc, argv);
            AddCommand(AppCommands::kInitVideo);
        }
    }

    void AppContext::SetController(unsigned int port, int retro_device) {
        core_->retro_set_controller_port_device(port, retro_device);
    }

}

/*-----App commands--------------------------------------------------------------*/
namespace libRetroRunner {
    void AppContext::processCommand() {
        std::shared_ptr<Command> command;
        while (command = command_queue_->Pop(), command.get() != nullptr) {
            int cmd = command->GetCommand();
            LOGD_APP("process command: %d", cmd);
            switch (cmd) {
                case AppCommands::kLoadCore: {
                    commandLoadCore();
                    return;
                }
                case AppCommands::kLoadContent: {
                    commandLoadContent();
                    return;
                }
                case AppCommands::kInitVideo: {
                    if (video_ && video_->Init()) {
                        BIT_SET(state, AppState::kVideoReady);
                    }
                    return;
                }
                case AppCommands::kUnloadVideo: {
                    if (environment_->GetRenderUseHWAcceleration())
                        environment_->InvokeRenderContextDestroy();
                    if (video_) {
                        video_->Destroy();
                        video_ = nullptr;
                    }
                    LOGD_APP("video unloaded");
                    BIT_DELETE(state, AppState::kVideoReady);
                    return;
                }
                case AppCommands::kInitInput: {
                    input_ = InputContext::Create(Setting::Current()->GetInputDriver());
                    input_->Init();
                    return;
                }
                case AppCommands::kResetGame: {
                    if (BIT_TEST(state, AppState::kContentReady)) {
                        core_->retro_reset();
                    }
                    return;
                }
                case AppCommands::kPauseGame: {
                    BIT_SET(state, AppState::kPaused);
                    return;
                }
                case AppCommands::kStopGame: {
                    BIT_DELETE(state, AppState::kRunning);
                    return;
                }
                case AppCommands::kInitAudio: {
                    if (!audio_) {
                        std::string audio_driver = Setting::Current()->GetAudioDriver();
                        audio_ = AudioContext::Create(audio_driver);
                        audio_->Init();
                        AddCommand(AppCommands::kEnableAudio);
                    }
                    return;
                }
                case AppCommands::kEnableAudio: {
                    if (audio_) {
                        audio_->Start();
                    }
                    return;
                }
                case AppCommands::kDisableAudio: {
                    if (audio_) {
                        audio_->Stop();
                    }
                    return;
                }
                case AppCommands::kNone:
                default:
                    break;
            }
        }
    }

    void AppContext::AddCommand(std::shared_ptr<Command> &command) {
        command_queue_->Push(command);
    }

    void AppContext::AddCommand(int command) {
        std::shared_ptr<Command> cmd = std::make_shared<Command>(command);
        command_queue_->Push(cmd);
    }

    void AppContext::commandLoadCore() {
        try {
            std::string core_path = paths_->GetCorePath();
            core_ = std::make_shared<Core>(core_path);

            core_->retro_set_video_refresh(&libretro_callback_hw_video_refresh);
            core_->retro_set_environment(&libretro_callback_set_environment);
            core_->retro_set_audio_sample(&libretro_callback_audio_sample);
            core_->retro_set_audio_sample_batch(&libretro_callback_audio_sample_batch);
            core_->retro_set_input_poll(&libretro_callback_input_poll);
            core_->retro_set_input_state(&libretro_callback_input_state);
            core_->retro_init();
            BIT_SET(state, AppState::kCoreReady);
            LOGD_APP("core loaded");
        } catch (std::exception &exception) {
            core_ = nullptr;
            LOGE_APP("load core failed");
        }
    }

    void AppContext::commandLoadContent() {
        if (BIT_TEST(state, AppState::kContentReady)) {
            LOGW_APP("content already loaded, ignore.");
            return;
        }
        if (!BIT_TEST(state, AppState::kCoreReady)) {
            LOGE_APP("try to load content, but core is not ready yet!!!");
            return;
        }

        std::string rom_path = paths_->GetRomPath();
        struct retro_system_info system_info{};
        core_->retro_get_system_info(&system_info);


        struct retro_game_info game_info{};
        game_info.path = rom_path.c_str();
        game_info.meta = nullptr;

        //TODO:这里需要加入zip解压支持的实现
        if (system_info.need_fullpath) {
            game_info.data = nullptr;
            game_info.size = 0;
        } else {
            struct Utils::ReadResult file = Utils::readFileAsBytes(rom_path.c_str());
            game_info.data = file.data;
            game_info.size = file.size;
        }

        bool result = core_->retro_load_game(&game_info);
        if (!result) {
            LOGE_APP("Cannot load game. Leaving.");
            throw std::runtime_error("Cannot load game");
        }

        //获取核心默认的尺寸
        struct retro_system_av_info avInfo;
        core_->retro_get_system_av_info(&avInfo);
        environment_->gameGeometryWidth = avInfo.geometry.base_width;
        environment_->gameGeometryHeight = avInfo.geometry.base_height;
        environment_->gameGeometryMaxWidth = avInfo.geometry.max_width;
        environment_->gameGeometryMaxHeight = avInfo.geometry.max_height;
        environment_->gameGeometryAspectRatio = avInfo.geometry.aspect_ratio;
        environment_->gameSampleRate = avInfo.timing.sample_rate;                //如果声音采样率变化了，则Audio组件可能需要重新初始化
        environment_->gameFps = avInfo.timing.fps;

        fps_time_throne_.SetFps(environment_->gameFps);

        BIT_SET(state, AppState::kContentReady);
        LOGD_APP("content loaded");
        AddCommand(AppCommands::kInitAudio);
        AddCommand(AppCommands::kInitInput);
    }


}

