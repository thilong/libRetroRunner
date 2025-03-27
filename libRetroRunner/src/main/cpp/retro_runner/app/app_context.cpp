//
// Created by Aidoo.TK on 2024/10/31.
//

#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <thread>
#include <future>

#include <libretro-common/include/libretro.h>

#include "environment.h"
#include "app_context.h"
#include "paths.h"
#include "setting.h"

#include <retro_runner/types/log.h>
#include <retro_runner/types/app_state.h>
#include <retro_runner/types/macros.h>
#include <retro_runner/core/core.h>
#include <retro_runner/utils/utils.h>
#include <retro_runner/video/video_context.h>
#include <retro_runner/input/input_context.h>
#include <retro_runner/audio/audio_context.h>
#include <retro_runner/types/error.h>

#ifdef ANDROID

#include <jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

namespace libRetroRunner {
    extern "C" JavaVM *gVm;
    extern "C" jobject gRRNativeRef;
}
#endif

#define LOGD_APP(...) LOGD("[APP] " __VA_ARGS__)
#define LOGW_APP(...) LOGW("[APP] " __VA_ARGS__)
#define LOGE_APP(...) LOGE("[APP] " __VA_ARGS__)
#define LOGI_APP(...) LOGI("[APP] " __VA_ARGS__)

/*-----RETRO CALLBACKS--------------------------------------------------------------*/
namespace libRetroRunner {
    void retroCallbackHwVideoRefresh(const void *data, unsigned int width, unsigned int height, size_t pitch) {
        auto appContext = AppContext::Current();
        if (appContext) {
            auto video = appContext->GetVideo();
            if (video) video->OnNewFrame(data, width, height, pitch);
        }
    }

    bool retroCallbackSetEnvironment(unsigned int cmd, void *data) {
        auto appContext = AppContext::Current();
        if (appContext) {
            return appContext->GetEnvironment()->HandleCoreCallback(cmd, data);
        }
        return false;
    }

    void retroCallbackAudioSample(int16_t left, int16_t right) {
        auto appContext = AppContext::Current();
        if (appContext) {
            auto audio = appContext->GetAudio();
            if (audio != nullptr) audio->OnAudioSample(left, right);
        }
    }

    size_t retroCallbackAudioSampleBatch(const int16_t *data, size_t frames) {
        auto appContext = AppContext::Current();
        if (appContext) {
            auto audio = appContext->GetAudio();
            if (audio != nullptr) audio->OnAudioSampleBatch(data, frames);
        }
        return frames;
    }

    void retroCallbackInputPoll(void) {
        auto appContext = AppContext::Current();
        if (appContext) {
            auto input = appContext->GetInput();
            if (input != nullptr) input->Poll();
        }
    }

    int16_t retroCallbackInputState(unsigned int port, unsigned int device, unsigned int index, unsigned int id) {
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
            LOGE_APP("AppContext already created.");
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
        memset(&app_window_, 0, sizeof(app_window_));
    }

    AppContext::~AppContext() {
        if (appInstance != nullptr && appInstance.get() == this) {
            appInstance = nullptr;
        }
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

    bool AppContext::Step() {
        processCommand();

        if (!BIT_TEST(state_, AppState::kRunning)) return false;

        //if emulate is paused, sleep for 16ms, for 60fps
        if (BIT_TEST(state_, AppState::kPaused)) {
            usleep(16000);
            return true;
        }

        //run core step when video and content are ready
        if (BIT_TEST(state_, AppState::kVideoReady) &&
            BIT_TEST(state_, AppState::kContentReady)) {
            //avoid emulator run too fast
            speed_limiter_.CheckAndWait(game_runtime_context_->GetFps());

            video_->Prepare();
            core_->retro_run();
        } else {
            usleep(16000);
        }
        return true;
    }

    void AppContext::Pause() {
        BIT_SET(state_, AppState::kPaused);
        AddCommand(AppCommands::kDisableAudio);
    }

    void AppContext::Resume() {
        BIT_UNSET(state_, AppState::kPaused);
        AddCommand(AppCommands::kEnableAudio);
    }

    void AppContext::Reset() {
        if (BIT_TEST(state_, kContentReady)) {
            AddCommand(AppCommands::kResetGame);
        } else {
            LOGE_APP("Try to reset game, but content not load yet.");
        }
    }

    void AppContext::Stop() {
        if (gettid() != emu_thread_id_) {
            LOGE_APP("AppContext::Stop should be called in emulation thread.");
        }
        //TODO: save sram
        //TODO: stop game

        BIT_UNSET(state_, AppState::kRunning);
        if (BIT_TEST(state_, AppState::kContentReady)) {
            core_->retro_unload_game();
            BIT_UNSET(state_, AppState::kContentReady);
        }
        if (BIT_TEST(state_, AppState::kCoreReady)) {
            core_->retro_deinit();
            BIT_UNSET(state_, AppState::kCoreReady);
        }


#ifdef ANDROID
        gVm->DetachCurrentThread();
#endif
    }

    void AppContext::Destroy() {
        if (gettid() == emu_thread_id_) {
            LOGE_APP("Can't destroy app context in the emulation thread.");
            return;
        }
        if (app_window_.window) {
            ANativeWindow_release(app_window_.window);
            app_window_.window = nullptr;
        }

        memset(&app_window_, 0, sizeof(app_window_));
        if (appInstance.get() == this) {
            appInstance = nullptr;
        }
    }

    void AppContext::CreateWithPaths(const std::string &rom, const std::string &core, const std::string &system, const std::string &save) {
        if (BIT_TEST(state_, AppState::kPathsReady)) {
            return;
        }
        game_runtime_context_ = std::make_shared<GameRuntimeContext>();
        game_runtime_context_->SetGamePath(rom);
        game_runtime_context_->SetSavePath(save);

        core_runtime_context_ = std::make_shared<CoreRuntimeContext>();
        core_runtime_context_->SetCorePath(core);
        core_runtime_context_->SetSystemPath(system);

        environment_ = std::make_shared<Environment>();
        environment_->SetGameRuntimeContext(game_runtime_context_);
        environment_->SetCoreRuntimeContext(core_runtime_context_);

        BIT_SET(state_, AppState::kPathsReady);
    }

    void AppContext::OnSurfaceChanged(void *env, void *surface, long surfaceId, unsigned int width, unsigned int height) {
        LOGI_APP("Surface %p Changed: %d x %d", surface, width, height);
        app_window_.width = width;
        app_window_.height = height;
        if (surface) {
            if (app_window_.surfaceId != surfaceId) {
                if (app_window_.window) {
                    ANativeWindow_release(app_window_.window);
                }
                app_window_.window = ANativeWindow_fromSurface((JNIEnv *) env, (jobject) surface);
                ANativeWindow_acquire(app_window_.window);
                app_window_.surface = (jobject) surface;
                app_window_.surfaceId = surfaceId;
                if (video_) {
                    AddCommand(AppCommands::kLoadVideo);
                }
            } else {
                if (video_) {
                    AddCommand(AppCommands::kUpdateVideoSize);
                }
            }
        } else {
            BIT_UNSET(state_, AppState::kVideoReady);
            if (app_window_.window) {
                ANativeWindow_release(app_window_.window);
                app_window_.window = nullptr;
            }
            app_window_.surface = nullptr;
            app_window_.surfaceId = 0;
            if (video_) {
                AddCommand(AppCommands::kUnloadVideo);
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
    int AppContext::addCommandWithPath(std::string path, int command, bool wait_for_result) {

        if (wait_for_result) {
            if (emu_thread_id_ == getpid()) {
                LOGE_APP("Can't add a command with waiting for result in the emu thread");
                return RRError::kBadOperation;
            }

            std::shared_ptr<ThreadCommand<int, std::string>> cmd = std::make_shared<ThreadCommand<int, std::string>>(command, path);
            std::shared_ptr<Command> baseCmd = cmd;
            this->AddCommand(baseCmd);
            cmd->Wait();
            int result = cmd->GetResult();
            return result;
        } else {
            std::shared_ptr<Command> cmd = std::make_shared<ParamCommand<std::string>>(command, path);
            this->AddCommand(cmd);
            LOGD_APP("command [%d] added: %s", command, path.c_str());
            return RRError::kSuccess;
        }
    }

    void AppContext::processCommand() {
        std::shared_ptr<Command> command;
        while (command = command_queue_->Pop(), command) {
            switch (command->GetCommand()) {
                case AppCommands::kInitApp: {
                    commandInitApp();
                    return;
                }
                case AppCommands::kLoadCore: {
                    commandLoadCore();
                    return;
                }
                case AppCommands::kLoadContent: {
                    commandLoadContent();
                    return;
                }
                case AppCommands::kInitComponents: {
                    commandInitComponents();
                    break;
                }

                case AppCommands::kLoadVideo: {
                    if (video_ && video_->Load()) {
                        BIT_SET(state_, AppState::kVideoReady);
                    }
                    return;
                }
                case AppCommands::kUnloadVideo: {
                    BIT_UNSET(state_, AppState::kVideoReady);
                    if (video_) {
                        video_->Unload();
                    }
                    return;
                }
                case AppCommands::kUpdateVideoSize: {
                    if (video_) {
                        video_->UpdateVideoSize(app_window_.width, app_window_.height);
                    }
                    return;
                }

                case AppCommands::kResetGame: {
                    if (BIT_TEST(state_, AppState::kContentReady)) {
                        core_->retro_reset();
                    }
                    return;
                }
                case AppCommands::kPauseGame: {
                    BIT_SET(state_, AppState::kPaused);
                    return;
                }
                case AppCommands::kStopGame: {
                    BIT_UNSET(state_, AppState::kRunning);
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
                        } else {
                            threadCommand->SetResult(RRError::kFailed);
                        }
                        threadCommand->Signal();
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
        return addCommandWithPath(path, AppCommands::kTakeScreenshot, wait_for_result);
    }

    int AppContext::AddSaveStateCommand(std::string &path, bool wait_for_result) {
        std::string savePath = path;
        return addCommandWithPath(savePath, AppCommands::kSaveState, wait_for_result);
    }

    int AppContext::AddLoadStateCommand(std::string &path, bool wait_for_result) {
        std::string savePath = path;
        return addCommandWithPath(savePath, AppCommands::kLoadState, wait_for_result);
    }

    int AppContext::AddSaveSRAMCommand(std::string &path, bool wait_for_result) {
        std::string savePath = path;
        return addCommandWithPath(savePath, AppCommands::kSaveSRAM, wait_for_result);
    }

    int AppContext::AddLoadSRAMCommand(std::string &path, bool wait_for_result) {
        std::string savePath = path;
        return addCommandWithPath(savePath, AppCommands::kLoadSRAM, wait_for_result);
    }

    void AppContext::commandInitApp() {
        emu_thread_id_ = gettid();
        BIT_SET(state_, AppState::kRunning);

#ifdef ANDROID
        gVm->AttachCurrentThread(&thread_jni_env_, nullptr);
#endif
    }

    void AppContext::commandLoadCore() {
        std::string core_path = core_runtime_context_->GetCorePath();
        try {
            core_ = std::make_shared<Core>(core_path);
            core_->retro_set_video_refresh(&retroCallbackHwVideoRefresh);
            core_->retro_set_environment(&retroCallbackSetEnvironment);
            core_->retro_set_audio_sample(&retroCallbackAudioSample);
            core_->retro_set_audio_sample_batch(&retroCallbackAudioSampleBatch);
            core_->retro_set_input_poll(&retroCallbackInputPoll);
            core_->retro_set_input_state(&retroCallbackInputState);
            core_->retro_init();
            BIT_SET(state_, AppState::kCoreReady);
            LOGD_APP("core loaded: %s", core_path.c_str());
        } catch (std::exception &exception) {
            core_ = nullptr;
            LOGE_APP("load core %s failed", core_path.c_str());
            BIT_UNSET(state_, AppState::kRunning);
        }
    }

    void AppContext::commandLoadContent() {
        if (BIT_TEST(state_, AppState::kContentReady)) {
            LOGW_APP("content already load, ignore.");
            return;
        }
        if (!BIT_TEST(state_, AppState::kCoreReady)) {
            LOGE_APP("try to load content, but core is not ready yet!!!");
            return;
        }

        std::string rom_path = game_runtime_context_->GetGamePath();
        struct retro_system_info system_info{};
        core_->retro_get_system_info(&system_info);

        std::vector<unsigned char> content_data;
        struct retro_game_info game_info{};
        game_info.path = rom_path.c_str();
        game_info.meta = nullptr;

        //TODO:这里需要加入zip解压支持的实现
        if (!system_info.need_fullpath) {
            content_data = Utils::readFileAsBytes(rom_path);
            game_info.data = &(content_data[0]);
            game_info.size = content_data.size();
        }

        bool result = core_->retro_load_game(&game_info);
        if (!result) {
            LOGE_APP("Cannot load game. Leaving.");
            BIT_UNSET(state_, AppState::kRunning);
            return;
        }

        //获取核心默认的尺寸
        struct retro_system_av_info avInfo;
        core_->retro_get_system_av_info(&avInfo);
        game_runtime_context_->SetGeometryWidth(avInfo.geometry.base_width);
        game_runtime_context_->SetGeometryHeight(avInfo.geometry.base_height);
        game_runtime_context_->SetGeometryMaxWidth(avInfo.geometry.max_width);
        game_runtime_context_->SetGeometryMaxHeight(avInfo.geometry.max_height);
        game_runtime_context_->SetGeometryAspectRatio(avInfo.geometry.aspect_ratio);
        game_runtime_context_->SetSampleRate(avInfo.timing.sample_rate);
        game_runtime_context_->SetFps(avInfo.timing.fps);

        BIT_SET(state_, AppState::kContentReady);
        LOGD_APP("content loaded");
    }

    void AppContext::commandInitComponents() {
        auto driver = Setting::Current()->GetVideoDriver();
        video_ = VideoContext::Create(driver, core_runtime_context_->GetRenderContextType());
        video_->SetGameContext(game_runtime_context_);

        auto audio_driver = Setting::Current()->GetAudioDriver();
        audio_ = AudioContext::Create(audio_driver);
        audio_->Init();
        AddCommand(AppCommands::kEnableAudio);

        input_ = InputContext::Create(Setting::Current()->GetInputDriver());
        input_->Init(core_runtime_context_->GetMaxUserCount());
        LOGD_APP("components initialized");

        if (app_window_.window) {
            AddCommand(AppCommands::kLoadVideo);
        }
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
            auto data = Utils::readFileAsBytes(savePath);
            if (!data.empty()) {
                LOGI("SRAM loaded: %s", savePath.c_str());
                memcpy(sramState, &(data[0]), data.size());
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
                LOGE_APP("serialize state to %s failed", savePath.c_str());
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
        auto data = Utils::readFileAsBytes(savePath);

        if (!data.empty()) {
            if (!core_->retro_unserialize(&(data[0]), data.size())) {
                LOGE_APP("can't unserialize state from %s ", savePath.c_str());
                ret = RRError::kCannotWriteData;
            } else {
                LOGI_APP("Unserialize state from %s complete.", savePath.c_str());
            }
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

namespace libRetroRunner {

    template<typename T>
    void AppContext::NotifyFrontend(FrontendNotify<T> *notify) {
        if (frontend_notify_) {
            notify->Retain();
            std::async(std::launch::async, [](FrontendNotifyCallback callback, FrontendNotify<T> *obj) {
                callback(obj);
                obj->Release();
            }, frontend_notify_, notify);
        }
    }

    void AppContext::NotifyFrontend(int notifyType) {
        //auto cmd = std::make_shared<int>(notifyType);
        auto notify = new FrontendNotify<int>(notifyType);
        this->NotifyFrontend(notify);
        notify->Release();
    }


}

