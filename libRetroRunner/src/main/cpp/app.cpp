//
// Created by aidoo on 2024/10/31.
//

#include <pthread.h>
#include <unistd.h>

#include "app.h"
#include "rr_log.h"
#include "utils/utils.h"
#include "misc/pref_counter.h"

#define LOGD_APP(...) LOGD("[Runner] " __VA_ARGS__)
#define LOGW_APP(...) LOGW("[Runner] " __VA_ARGS__)
#define LOGE_APP(...) LOGE("[Runner] " __VA_ARGS__)
#define LOGI_APP(...) LOGI("[Runner] " __VA_ARGS__)

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
        save_path = "";

        input = nullptr;
        core = nullptr;
        video = nullptr;
        audio == nullptr;
        environment = nullptr;
    }

    void AppContext::SetFiles(const std::string &romPath, const std::string &corePath, const std::string &systemPath, const std::string &savePath) {
        this->rom_path = romPath;
        this->core_path = corePath;
        this->system_path = systemPath;
        this->save_path = savePath;
        this->environment = std::make_unique<Environment>();
        environment->SetSavePath(save_path);
        environment->SetSystemPath(system_path);

        BIT_SET(state, AppState::kPathSet);
    }

    void AppContext::SetVariable(const std::string &key, const std::string &value, bool notifyCore) {
        environment->UpdateVariable(key, value, notifyCore);
    }

    void AppContext::SetVideoRenderTarget(void **args, int argc) {
        if (args == nullptr) {
            BIT_DELETE(state, AppState::kVideoReady);
            this->AddCommand(AppCommands::kUnloadVideo);
        } else {
            LOGW_APP("create new video component");
            //create video
            this->video = VideoContext::NewInstance();
            this->video->SetHolder(args[0], args[1]);
            this->AddCommand(AppCommands::kInitVideo);
        }
    }

    void AppContext::SetVideoRenderSize(unsigned int width, unsigned int height) {
        if (video != nullptr) {
            video->SetSurfaceSize(width, height);
        }
    }

    void AppContext::Start() {
        if (!BIT_TEST(state, AppState::kPathSet)) {
            LOGE_APP("Paths are empty , can't start");
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
        AddCommand(AppCommands::kEnableAudio);
        AddCommand(AppCommands::kStopGame);

    }

    void AppContext::ThreadLoop() {
        BIT_SET(state, AppState::kRunning);
        JNIEnv *env;
        PrefCounter counter;
        try {
            while (BIT_TEST(state, AppState::kRunning)) {
                //先处理命令
                processCommand();
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
        BIT_DELETE(state, AppState::kRunning);
        LOGW_APP("emu stopped");
    }
}

/*-----功能控制相关--------------------------------------------------------------*/
namespace libRetroRunner {
    void AppContext::SetController(int port, int device) {
        core->retro_set_controller_port_device(port, device);
    }
}

/*-----App Commands--------------------------------------------------------------*/
namespace libRetroRunner {

    //这个函数只在模拟线程(绘图线程,android中为OPENGL所在线程)中调用
    void AppContext::processCommand() {
        int command;
        while (commands.try_pop(command)) {
            LOGW_APP("process command: %d", command);
            switch (command) {
                case AppCommands::kLoadCore:
                    cmdLoadCore();
                    break;
                case AppCommands::kLoadContent:
                    cmdLoadContent();
                    break;
                case AppCommands::kInitVideo:
                    cmdInitVideo();
                    break;
                case AppCommands::kUnloadVideo:
                    cmdUnloadVideo();
                    break;
                case AppCommands::kInitInput:
                    cmdInitInput();
                    break;
                case AppCommands::kResetGame:
                    core->retro_reset();
                    break;
                case AppCommands::kPauseGame:
                    BIT_SET(state, AppState::kPaused);
                    break;
                case AppCommands::kStopGame:
                    BIT_DELETE(state, AppState::kRunning);
                    break;
                case AppCommands::kInitAudio:
                    cmdInitAudio();
                    break;

                case AppCommands::kEnableAudio:
                    if (audio) audio->Start();
                    break;
                case AppCommands::kDisableAudio:
                    if (audio) audio->Stop();
                    break;
                case AppCommands::kNone:
                default:
                    break;
            }
        }
    }

    void AppContext::AddCommand(int command) {
        commands.push(command);
    }

    void AppContext::cmdLoadCore() {
        LOGD_APP("cmd: load core -> %s", core_path.c_str());
        try {
            core = std::make_unique<Core>(this->core_path);

            core->retro_set_video_refresh(&RetroCallbacks::libretro_callback_hw_video_refresh);
            core->retro_set_environment(&RetroCallbacks::libretro_callback_set_environment);
            core->retro_set_audio_sample(&RetroCallbacks::libretro_callback_audio_sample);
            core->retro_set_audio_sample_batch(&RetroCallbacks::libretro_callback_audio_sample_batch);
            core->retro_set_input_poll(&RetroCallbacks::libretro_callback_input_poll);
            core->retro_set_input_state(&RetroCallbacks::libretro_callback_input_state);

            core->retro_init();


            BIT_SET(state, AppState::kCoreReady);
        } catch (std::exception &exception) {
            core = nullptr;
            LOGE_APP("load core failed");
        }
    }

    void AppContext::cmdLoadContent() {
        LOGD_APP("cmd: load content: %s", rom_path.c_str());
        if (BIT_TEST(state, AppState::kContentReady)) {
            LOGW_APP("content already loaded, ignore.");
            return;
        }
        if (!BIT_TEST(state, AppState::kCoreReady)) {
            LOGE_APP("try to load content, but core is not ready yet!!!");
            return;
        }

        struct retro_system_info system_info{};
        core->retro_get_system_info(&system_info);

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

        bool result = core->retro_load_game(&game_info);
        if (!result) {
            LOGE_APP("Cannot load game. Leaving.");
            throw std::runtime_error("Cannot load game");
        }

        //获取核心默认的尺寸
        struct retro_system_av_info avInfo;
        core->retro_get_system_av_info(&avInfo);
        environment->gameGeometryWidth = avInfo.geometry.base_width;
        environment->gameGeometryHeight = avInfo.geometry.base_height;
        environment->gameGeometryMaxWidth = avInfo.geometry.max_width;
        environment->gameGeometryMaxHeight = avInfo.geometry.max_height;
        environment->gameGeometryAspectRatio = avInfo.geometry.aspect_ratio;
        environment->gameSampleRate = avInfo.timing.sample_rate;                //如果声音采样率变化了，则Audio组件可能需要重新初始化
        environment->gameFps = avInfo.timing.fps;

        BIT_SET(state, AppState::kContentReady);

        AddCommand(AppCommands::kInitInput);
        AddCommand(AppCommands::kInitAudio);
    }

    void AppContext::cmdInitInput() {
        if (input == nullptr) {
            input = InputContext::NewInstance();
            input->Init();
        }

        /*TODO: 这里需要根据平台不同返回不一样的设备信息，比如PS有可能需要RETRO_DEVICE_ANALOG
            这里有可能需要先在environment中获取由核心传回的控制器信息来决定
            @see RETRO_ENVIRONMENT_SET_CONTROLLER_INFO
         */
        for (int player = 0; player < MAX_PLAYER; player++) {
            core->retro_set_controller_port_device(player, RETRO_DEVICE_JOYPAD);
        }
    }

    void AppContext::cmdInitVideo() {
        LOGD_APP("cmd: init video");
        JNIEnv *env;
        gVm->AttachCurrentThread(&env, nullptr);
        video->Init();
        gVm->DetachCurrentThread();
        BIT_SET(state, AppState::kVideoReady);

    }

    void AppContext::cmdUnloadVideo() {
        LOGD_APP("cmd: unload video");
        JNIEnv *env;
        gVm->AttachCurrentThread(&env, nullptr);
        video->Destroy();
        video = nullptr;
        if (environment->renderUseHWAcceleration) {
            environment->renderContextDestroy();
        }
        gVm->DetachCurrentThread();
    }

    void AppContext::cmdInitAudio() {
        LOGD_APP("cmd: init audio");
        if (audio == nullptr) {
            audio = AudioContext::NewInstance();
            audio->Init();
            AddCommand(AppCommands::kEnableAudio);
        }
    }

}

/*-----RETRO CALLBACKS--------------------------------------------------------------*/
namespace libRetroRunner {
    void RetroCallbacks::libretro_callback_hw_video_refresh(const void *data, unsigned int width, unsigned int height, size_t pitch) {
        AppContext *appContext = AppContext::Current();
        if (appContext) {
            VideoContext *video = appContext->GetVideo();
            if (video) video->OnNewFrame(data, width, height, pitch);
        }
    }

    bool RetroCallbacks::libretro_callback_set_environment(unsigned int cmd, void *data) {
        AppContext *appContext = AppContext::Current();
        if (appContext) {
            return appContext->GetEnvironment()->HandleCoreCallback(cmd, data);
        }
        return false;
    }

    void RetroCallbacks::libretro_callback_audio_sample(int16_t left, int16_t right) {
        AppContext *appContext = AppContext::Current();
        if (appContext) {
            auto audio = appContext->GetAudio();
            if (audio != nullptr) audio->OnAudioSample(left, right);
        }
    }

    size_t RetroCallbacks::libretro_callback_audio_sample_batch(const int16_t *data, size_t frames) {
        AppContext *appContext = AppContext::Current();
        if (appContext) {
            auto audio = appContext->GetAudio();
            if (audio != nullptr) audio->OnAudioSampleBatch(data, frames);
        }
        return frames;
    }

    void RetroCallbacks::libretro_callback_input_poll(void) {
        auto appContext = AppContext::Current();
        if (appContext) {
            auto input = appContext->GetInput();
            if (input != nullptr) input->Poll();
        }
    }

    int16_t RetroCallbacks::libretro_callback_input_state(unsigned int port, unsigned int device, unsigned int index, unsigned int id) {
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

