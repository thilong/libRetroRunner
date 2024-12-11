//
// Created by aidoo on 2024/10/31.
//

#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <thread>

#include <libretro-common/include/libretro.h>

#include "environment.h"
#include "app_context.h"
#include "paths.h"
#include "setting.h"

#include "../types/log.h"
#include "../types/app_state.h"
#include "../types/macros.h"
#include "../core/core.h"
#include "../utils/utils.h"
#include "../video/video_context.h"
#include "../input/input_context.h"
#include "../audio/audio_context.h"
#include "../types/error.h"

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

    static std::shared_ptr<AppContext> appInstance(nullptr);

    std::shared_ptr<AppContext> AppContext::CreateNew() {
        if (appInstance != nullptr) {
            appInstance->Stop();
        }

        appInstance = std::make_shared<AppContext>();
        return appInstance;
    }

    std::shared_ptr<AppContext> AppContext::Current() {
        return appInstance;
    }

    AppContext::AppContext() {
        command_queue_ = std::make_unique<CommandQueue>();
        core_ = nullptr;
        environment_ = nullptr;
        video_ = nullptr;
        input_ = nullptr;
        audio_ = nullptr;

        core_runtime_context_ = nullptr;
        game_runtime_context_ = nullptr;
        emu_thread_id_ = -1;
    }

    AppContext::~AppContext() {
        if (appInstance != nullptr && appInstance.get() == this) {
            appInstance = nullptr;
        }
    }

    void AppContext::ThreadLoop() {
        emu_thread_id_ = gettid();
        BIT_SET(state, AppState::kRunning);
        try {
            while (BIT_TEST(state, AppState::kRunning)) {
                processCommand();
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
                    speed_limiter_.CheckAndWait(game_runtime_context_->get_fps());

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
}

namespace libRetroRunner {
    std::shared_ptr<class Core> AppContext::GetCore() const {
        return core_;
    }

    std::shared_ptr<class Environment> AppContext::GetEnvironment() const {
        return environment_;
    }

    std::shared_ptr<class VideoContext> AppContext::GetVideo() const {
        return video_;
    }

    std::shared_ptr<class InputContext> AppContext::GetInput() const {
        return input_;
    }

    std::shared_ptr<class AudioContext> AppContext::GetAudio() const {
        return audio_;
    }

    std::shared_ptr<CoreRuntimeContext> AppContext::GetCoreRuntimeContext() const {
        return core_runtime_context_;
    }

    std::shared_ptr<GameRuntimeContext> AppContext::GetGameRuntimeContext() const {
        return game_runtime_context_;
    }
}

/*-----Emulator control--------------------------------------------------------------*/
namespace libRetroRunner {

    void AppContext::Start() {
        if (state >= AppState::kRunning) {
            LOGE_APP("Emulator is already started.");
            return;
        }

        if (!BIT_TEST(state, AppState::kPathsReady)) {
            LOGE_APP("Paths are not set yet , can't start emulator.");
            return;
        }
        AddCommand(AppCommands::kLoadCore);
        AddCommand(AppCommands::kLoadContent);
        std::thread this_thread([](AppContext *app) {
            app->ThreadLoop();
            LOGD_APP("emu thread exit.");
        }, this);
        this_thread.detach();
        LOGD_APP("emu started, app: %p, thread result: %ld", this, this_thread.native_handle());
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
        if (BIT_TEST(state, AppState::kPathsReady)) {
            return;
        }
        game_runtime_context_ = std::make_shared<GameRuntimeContext>();
        game_runtime_context_->set_game_path(rom);
        game_runtime_context_->set_save_path(save);

        core_runtime_context_ = std::make_shared<CoreRuntimeContext>();
        core_runtime_context_->set_core_path(core);
        core_runtime_context_->set_system_path(system);

        environment_ = std::make_shared<Environment>();

        BIT_SET(state, AppState::kPathsReady);
    }

    void AppContext::SetVideoSurface(int argc, void **argv) {
        if (argc < 1) {
            LOGE_APP("SetVideoSurface: invalid arguments");
            return;
        }
        if (argv[1] == nullptr) {
            BIT_DELETE(state, AppState::kVideoReady);
            AddCommand(AppCommands::kUnloadVideo);
            return;
        } else {
            std::string driver = Setting::Current()->GetVideoDriver();
            video_ = VideoContext::Create(driver);
            if (video_) {
                video_->SetSurface(argc, argv);
                AddCommand(AppCommands::kInitVideo);
            }
        }
    }

    void AppContext::SetController(unsigned int port, int retro_device) {
        if (core_)
            core_->retro_set_controller_port_device(port, retro_device);
    }

}

/*-----App commands--------------------------------------------------------------*/
namespace libRetroRunner {
    int AppContext::sendCommandWithPath(std::string path, int command, bool wait_for_result) {
        if (wait_for_result && (emu_thread_id_ == getpid())) {
            throw std::runtime_error("Can't add a command with waiting for result in the emu thread");
        }
        if (wait_for_result) {
            std::shared_ptr<ThreadCommand<int, std::string>> cmd = std::make_shared<ThreadCommand<int, std::string>>(command, path);
            std::shared_ptr<Command> baseCmd = cmd;
            this->AddCommand(baseCmd);
            cmd->Wait();
            int result = cmd->GetResult();
            LOGD_APP("command [%d] sent and wait , path: %s , result: %s", command, path.c_str(), result == RRError::kSuccess ? "success" : "failed");
            return result;
        } else {
            std::shared_ptr<Command> cmd = std::make_shared<ParamCommand<std::string>>(AppCommands::kTakeScreenshot, path);
            this->AddCommand(cmd);
            LOGD_APP("command [%d] added: %s", command, path.c_str());
            return RRError::kSuccess;
        }
    }

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
                case AppCommands::kTakeScreenshot: {
                    if (command->GetCommandType() == CommandType::kThreadCommand) {
                        std::shared_ptr<ThreadCommand<int, std::string>> threadCommand = std::static_pointer_cast<ThreadCommand<int, std::string>>(command);
                        if (video_) {
                            std::string savePath = threadCommand->GetArg();
                            bool result = video_->TakeScreenshot(savePath);
                            threadCommand->SetResult(result ? RRError::kSuccess : RRError::kFailed);
                            threadCommand->Signal();
                        }
                    } else {
                        if (video_) {
                            std::shared_ptr<ParamCommand<std::string>> paramCommand = std::static_pointer_cast<ParamCommand<std::string>>(command);
                            std::string savePath = paramCommand->GetArg();
                            video_->SetNextScreenshotStorePath(savePath);
                        }
                    }
                    return;
                }
                case AppCommands::kSaveSRAM: {
                    commandSaveSRAM(command);
                    return;
                }
                case AppCommands::kLoadSRAM: {
                    commandLoadSRAM(command);
                    return;
                }
                case AppCommands::kSaveState: {
                    commandSaveState(command);
                    return;
                }
                case AppCommands::kLoadState: {
                    commandLoadState(command);
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

    int AppContext::AddTakeScreenshotCommand(std::string &path, bool wait_for_result) {
        return sendCommandWithPath(path, AppCommands::kTakeScreenshot, wait_for_result);
    }

    int AppContext::AddSaveStateCommand(std::string &path, bool wait_for_result) {
        std::string savePath = path;
        if (savePath.empty()) {
            savePath = paths_->GetSaveStatePath(0);
        }
        return sendCommandWithPath(savePath, AppCommands::kSaveState, wait_for_result);
    }

    int AppContext::AddLoadStateCommand(std::string &path, bool wait_for_result) {
        std::string savePath = path;
        if (savePath.empty()) {
            savePath = paths_->GetSaveStatePath(0);
        }
        return sendCommandWithPath(savePath, AppCommands::kLoadState, wait_for_result);
    }

    int AppContext::AddSaveSRAMCommand(std::string &path, bool wait_for_result) {
        std::string savePath = path;
        if (savePath.empty()) {
            savePath = paths_->GetSaveRamPath();
        }
        return sendCommandWithPath(savePath, AppCommands::kSaveSRAM, wait_for_result);
    }

    int AppContext::AddLoadSRAMCommand(std::string &path, bool wait_for_result) {
        std::string savePath = path;
        if (savePath.empty()) {
            savePath = paths_->GetSaveRamPath();
        }
        return sendCommandWithPath(savePath, AppCommands::kLoadSRAM, wait_for_result);
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

        speed_limiter_.SetFps(environment_->gameFps);

        BIT_SET(state, AppState::kContentReady);
        LOGD_APP("content loaded");
        AddCommand(AppCommands::kInitAudio);
        AddCommand(AppCommands::kInitInput);
    }

    void AppContext::commandSaveSRAM(std::shared_ptr<Command> &command) {
        std::shared_ptr<ParamCommand<std::string>> paramCommand = std::static_pointer_cast<ParamCommand<std::string>>(command);
        std::string savePath = paramCommand->GetArg();

        size_t ramSize = core_->retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        unsigned char *ramData = (unsigned char *) core_->retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        int ret = RRError::kSuccess;
        if (ramData == nullptr || ramSize == 0) {
            LOGW_APP("ram data is empty, can't save ram");
            ret = RRError::kEmptyMemory;
        }
        size_t wrote = Utils::writeBytesToFile(savePath, (char *) ramData, ramSize);
        if (wrote != 0 && wrote == ramSize) {
            LOGD_APP("save ram to %s, size: %d", savePath.c_str(), wrote);
        } else {
            ret = kCannotWriteData;
        }

        if (command->GetCommandType() == CommandType::kThreadCommand) {
            std::shared_ptr<ThreadCommand<int, std::string>> threadCommand = std::static_pointer_cast<ThreadCommand<int, std::string>>(command);
            threadCommand->SetResult(ret);
            threadCommand->Signal();
        }
    }

    void AppContext::commandLoadSRAM(std::shared_ptr<Command> &command) {
        int ret = RRError::kSuccess;
        size_t sramSize = core_->retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        void *sramState = core_->retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);

        if (sramState == nullptr) {
            LOGE("Cannot load SRAM: empty in core...");
            ret = RRError::kEmptyMemory;
        } else {
            std::shared_ptr<ParamCommand<std::string>> paramCommand = std::static_pointer_cast<ParamCommand<std::string>>(command);
            std::string savePath = paramCommand->GetArg();
            Utils::ReadResult data = Utils::readFileAsBytes(savePath);
            if (data.data) {
                LOGI("SRAM loaded: %s", savePath.c_str());
                memcpy(sramState, data.data, data.size);
                delete[] data.data;
            } else {
                LOGE("Cannot load SRAM: empty file: %s", savePath.c_str());
                ret = RRError::kEmptyFile;
            }
        }

        if (command->GetCommandType() == CommandType::kThreadCommand) {
            std::shared_ptr<ThreadCommand<int, std::string>> threadCommand = std::static_pointer_cast<ThreadCommand<int, std::string>>(command);
            threadCommand->SetResult(ret);
            threadCommand->Signal();
        }

    }

    void AppContext::commandSaveState(std::shared_ptr<Command> &command) {
        std::shared_ptr<ParamCommand<std::string>> paramCommand = std::static_pointer_cast<ParamCommand<std::string>>(command);
        std::string savePath = paramCommand->GetArg();

        size_t stateSize = core_->retro_serialize_size();
        int ret = RRError::kSuccess;
        if (stateSize == 0) {
            LOGW_APP("state data is empty, can't save state");
            ret = RRError::kEmptyMemory;
        } else {
            std::unique_ptr<unsigned char> stateData(new unsigned char[stateSize]);
            if (!core_->retro_serialize(stateData.get(), stateSize)) {
                LOGE_APP("serialize state to %d failed", savePath.c_str());
                ret = RRError::kCannotReadMemory;
            } else {
                int wroteSize = Utils::writeBytesToFile(savePath, (char *) stateData.get(), stateSize);
                if (wroteSize != 0 && wroteSize == stateSize) {
                    LOGI_APP("save state to %s, size: %d", savePath.c_str(), wroteSize);
                } else {
                    ret = kCannotWriteData;
                }
            }
        }

        if (command->GetCommandType() == CommandType::kThreadCommand) {
            std::shared_ptr<ThreadCommand<int, std::string>> threadCommand = std::static_pointer_cast<ThreadCommand<int, std::string>>(command);
            threadCommand->SetResult(ret);
            threadCommand->Signal();
        }
    }

    void AppContext::commandLoadState(std::shared_ptr<Command> &command) {
        int ret = RRError::kSuccess;

        std::shared_ptr<ParamCommand<std::string>> paramCommand = std::static_pointer_cast<ParamCommand<std::string>>(command);
        std::string savePath = paramCommand->GetArg();
        Utils::ReadResult data = Utils::readFileAsBytes(savePath);


        if (data.data) {
            if (!core_->retro_unserialize(data.data, data.size)) {
                LOGE_APP("can't unserialize state from %s ", savePath.c_str());
                ret = RRError::kCannotWriteData;
            } else {
                LOGI_APP("Unserialize state from %s complete.", savePath.c_str());
            }
            delete[] data.data;
        } else {
            LOGE("Cannot load state: empty file: %s", savePath.c_str());
            ret = RRError::kEmptyFile;
        }


        if (command->GetCommandType() == CommandType::kThreadCommand) {
            std::shared_ptr<ThreadCommand<int, std::string>> threadCommand = std::static_pointer_cast<ThreadCommand<int, std::string>>(command);
            threadCommand->SetResult(ret);
            threadCommand->Signal();
        }
    }


}

