//
// Created by aidoo on 2024/10/31.
//

#include <pthread.h>
#include <unistd.h>

#include "app.h"
#include "rr_log.h"
#include "utils/utils.h"
#include "utils/pref_counter.h"

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

    template<typename T>
    void threadCommandCheckAndSetResultAndNotify(std::shared_ptr<Command> command, T result) {
        if (command->threaded) {
            std::shared_ptr<ThreadCommand<T>> threadCommand = std::static_pointer_cast<ThreadCommand<T>>(command);
            threadCommand->SetResult((result));
            threadCommand->Signal();
        }
    }

    template<typename R, typename T>
    void threadParamCommandCheckAndSetResultAndNotify(std::shared_ptr<Command> command, T result) {
        if (command->threaded) {
            std::shared_ptr<ThreadParamCommand<R, T>> threadCommand = std::static_pointer_cast<ThreadParamCommand<R, T>>(command);
            threadCommand->SetResult((result));
            threadCommand->Signal();
        }
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

    void AppContext::SetFiles(const std::string &romPath, const std::string &corePath, const std::string &systemPath, const std::string &savePath) {
        this->rom_path = romPath;
        this->core_path = corePath;
        this->system_path = systemPath;
        this->environment = std::make_unique<Environment>();
        this->setting = std::make_unique<Setting>();
        this->cheatManager = std::make_unique<CheatManager>();

        environment->SetSavePath(savePath);
        environment->SetSystemPath(system_path);
        this->setting->InitWithPaths(romPath, savePath);
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
        AddCommand(AppCommands::kSaveSRAM);
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
                if (!BIT_TEST(state, AppState::kRunning)) break;

                //时间计算, 保持帧率
                timeThrone.CheckAndWait(environment->GetFastForwardFps());

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

/*-----功能控制相关--------------------------------------------------------------*/
namespace libRetroRunner {
    void AppContext::SetController(int port, int device) {
        core->retro_set_controller_port_device(port, device);
    }

    bool AppContext::SaveRAM() {
        const std::string savePath = setting->saveRamPath;
        if (savePath.empty()) {
            LOGW_APP("save path is empty, can't save ram");
            return false;
        }

        size_t ramSize = core->retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        unsigned char *ramData = (unsigned char *) core->retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        if (ramData == nullptr || ramSize == 0) {
            LOGW_APP("ram data is empty, can't save ram");
            return false;
        }
        size_t wrote = Utils::writeBytesToFile(savePath, (char *) ramData, ramSize);
        if (wrote != 0 && wrote == ramSize) {
            LOGD_APP("save ram to %s, size: %d", savePath.c_str(), wrote);
        }
        return wrote == ramSize && wrote != 0;
    }

    bool AppContext::LoadRAM() {
        const std::string savePath = setting->saveRamPath;
        if (savePath.empty()) {
            LOGW_APP("save path is empty, can't save ram");
            return false;
        }

        size_t sramSize = core->retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        void *sramState = core->retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        if (sramState == nullptr) {
            LOGE("Cannot load SRAM: empty in core...");
            return false;
        }
        Utils::ReadResult data = Utils::readFileAsBytes(savePath);
        if (data.data) {
            memcpy(sramState, data.data, data.size);
            delete[] data.data;
        } else {
            LOGE("Cannot load SRAM: empty in file...");
            return false;
        }
        return true;
    }

    bool AppContext::SaveState(int idx) {
        size_t stateSize = core->retro_serialize_size();
        if (stateSize == 0) {
            LOGW_APP("state size is 0, can't save state %d", idx);
            return false;
        }
        std::unique_ptr<unsigned char> stateData(new unsigned char[stateSize]);
        if (!core->retro_serialize(stateData.get(), stateSize)) {
            LOGW_APP("serialize state %d failed", idx);
            return false;
        }
        std::string stateFile = setting->getSaveStatePath(idx);
        int wroteSize = Utils::writeBytesToFile(stateFile, (char *) stateData.get(), stateSize);
        return wroteSize == stateSize;
    }

    bool AppContext::LoadState(int idx) {
        std::string stateFile = setting->getSaveStatePath(idx);
        Utils::ReadResult data = Utils::readFileAsBytes(stateFile);
        if (data.data == nullptr) {
            LOGE_APP("Cannot load state %d: file is empty or not found", idx);
            return false;
        }

        if (!core->retro_unserialize(data.data, data.size)) {
            LOGE_APP("Unserialize state %d failed", idx);
            delete[] data.data;
            return false;
        }

        delete[] data.data;
        return true;
    }

    void AppContext::SetFastForward(bool enable) {
        //TODO: 这里需要实现快进功能
    }

}

/*-----App Commands--------------------------------------------------------------*/
namespace libRetroRunner {

    //这个函数只在模拟线程(绘图线程,android中为OPENGL所在线程)中调用
    void AppContext::processCommand() {
        std::shared_ptr<Command> command;
        while (command = commands.Pop(), command.get() != nullptr) {
            int cmd = command->command;
            LOGW_APP("process command: %d", cmd);
            switch (cmd) {
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
                case AppCommands::kTakeScreenshot: {
                    std::shared_ptr<ParamCommand<std::string>> paramCommand = std::static_pointer_cast<ParamCommand<std::string>>(command);
                    std::string savePath = paramCommand->GetArg();
                    if (video)
                        video->SetTakeScreenshot(savePath);
                    else
                        LOGW_APP("video is not ready, can't take screenshot");
                    break;
                }
                case AppCommands::kSaveSRAM: {
                    bool saveResult = SaveRAM();
                    threadCommandCheckAndSetResultAndNotify<int>(command, saveResult ? 0 : -1);
                    break;
                }
                case AppCommands::kLoadSRAM: {
                    bool saveResult = LoadRAM();
                    threadCommandCheckAndSetResultAndNotify<int>(command, saveResult ? 0 : -1);
                    break;
                }
                case AppCommands::kSaveState: {
                    int idx = 0;
                    if (command->threaded) {
                        std::shared_ptr<ThreadParamCommand<int, int>> threadCommand = std::static_pointer_cast<ThreadParamCommand<int, int>>(command);
                        idx = threadCommand->GetArg();
                    }
                    bool saveResult = SaveState(idx);
                    threadParamCommandCheckAndSetResultAndNotify<int, int>(command, saveResult ? 0 : -1);
                    break;
                }
                case AppCommands::kLoadState: {
                    int idx = 0;
                    if (command->threaded) {
                        std::shared_ptr<ThreadParamCommand<int, int>> threadCommand = std::static_pointer_cast<ThreadParamCommand<int, int>>(command);
                        idx = threadCommand->GetArg();
                    }
                    bool saveResult = LoadState(idx);
                    threadParamCommandCheckAndSetResultAndNotify<int, int>(command, saveResult ? 0 : -1);
                    break;
                }
                case AppCommands::kLoadCheats: {
                    std::shared_ptr<Command> command = std::make_shared<Command>(AppCommands::kLoadCheats);
                    if (!command->threaded)
                        cheatManager->LoadFromSetting();
                    else {
                        std::shared_ptr<ThreadParamCommand<bool, std::string>> paramCommand = std::static_pointer_cast<ThreadParamCommand<bool, std::string>>(command);
                        std::string path = paramCommand->GetArg();
                        cheatManager->LoadFromFile(path);
                        paramCommand->Signal();
                    }
                    break;
                }
                case AppCommands::kNone:
                default:
                    break;
            }
        }
    }

    void AppContext::AddCommand(int command) {
        std::shared_ptr<Command> newCommand = std::make_shared<Command>(command);
        commands.Push(newCommand);
    }

    void AppContext::AddThreadCommand(std::shared_ptr<Command> &command) {
        commands.Push(command);
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

        AddCommand(AppCommands::kLoadSRAM);
        AddCommand(AppCommands::kLoadCheats);
        AddCommand(AppCommands::kInitInput);
        AddCommand(AppCommands::kInitAudio);
    }

    void AppContext::cmdInitInput() {
        if (input == nullptr) {
            input = InputContext::NewInstance();
            input->Init();
        }
    }

    void AppContext::cmdInitVideo() {
        LOGD_APP("cmd: init video");
        //JNIEnv *env;
        //gVm->AttachCurrentThread(&env, nullptr);
        video->Init();
        //gVm->DetachCurrentThread();
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

