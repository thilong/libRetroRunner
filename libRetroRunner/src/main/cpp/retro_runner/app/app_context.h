//
// Created by Aidoo.TK on 2024/10/31.
//

#ifndef _APP_H
#define _APP_H

#include <string>
#include <retro_runner/types/app_command.hpp>
#include <retro_runner/runtime_contexts/core_context.h>
#include <retro_runner/runtime_contexts/game_context.h>
#include <retro_runner/types/frontend_notify.hpp>
#include <retro_runner/app/speed_limiter.hpp>

#ifdef ANDROID

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#endif

namespace libRetroRunner {

    struct AppWindow {
#ifdef ANDROID
        ANativeWindow *window;
        jobject surface;
#else
        void *window;
        void *surface;
#endif
        unsigned width;
        unsigned height;
    };


    class AppContext {

    public:
        AppContext();

        ~AppContext();

        static std::shared_ptr<AppContext> CreateNew();

        static std::shared_ptr<AppContext> Current();


    public:
        inline void SetFrontendNotify(FrontendNotifyCallback notify) {
            frontend_notify_ = notify;
        }

        std::shared_ptr<class Environment> GetEnvironment() const;

        std::shared_ptr<class VideoContext> GetVideo() const;

        std::shared_ptr<class InputContext> GetInput() const;

        std::shared_ptr<class AudioContext> GetAudio() const;

        std::shared_ptr<class Core> GetCore() const;

        std::shared_ptr<CoreRuntimeContext> GetCoreRuntimeContext() const;

        std::shared_ptr<GameRuntimeContext> GetGameRuntimeContext() const;

        long GetState() { return state_; }

        AppWindow &GetAppWindow() { return app_window_; }

#ifdef ANDROID

        JNIEnv *GetJniEnv() const { return thread_jni_env_; };

#endif


    public:

        /**
         * Set the appropriate paths for retro runner
         * @param rom       game file
         * @param core      core file
         * @param system    system folder for core
         * @param save      folder to save game states, ram, screenshots
         */
        void CreateWithPaths(const std::string &rom, const std::string &core, const std::string &system, const std::string &save);;

        bool Step();

        void Pause();

        void Resume();

        void Reset();

        /**
         * Stop emu app context, clean up everything.
         */
        void Stop();

        void OnSurfaceChanged(void *env, void *surface, unsigned width, unsigned height);

        void SetController(unsigned port, int retro_device);


    public:
        void AddCommand(int command);

        void AddCommand(std::shared_ptr<Command> &command);

        int AddTakeScreenshotCommand(std::string &path, bool wait_for_result = false);

        int AddSaveStateCommand(std::string &path, bool wait_for_result = false);

        int AddLoadStateCommand(std::string &path, bool wait_for_result = false);

        int AddSaveSRAMCommand(std::string &path, bool wait_for_result = false);

        int AddLoadSRAMCommand(std::string &path, bool wait_for_result = false);

    private:
        /**
         * Add a command to the command queue, if wait_for_result is true, this will block until the command is processed,
         * otherwise it will return immediately. So, you should not add a command with wait_for_result = true in the emu thread.
         * @param path  path param
         * @param command command id
         * @param wait_for_result   wait for result or not
         * @return  error code or 0 for success
         */
        int addCommandWithPath(std::string path, int command, bool wait_for_result = false);

        void processCommand();

        void commandInitApp();

        void commandLoadCore();

        void commandLoadContent();

        void commandInitComponents();

        void commandSaveSRAM(std::shared_ptr<Command> &command);

        void commandLoadSRAM(std::shared_ptr<Command> &command);

        void commandSaveState(std::shared_ptr<Command> &command);

        void commandLoadState(std::shared_ptr<Command> &command);

    public:
        template<class T>
        void NotifyFrontend(FrontendNotify<T> *notify);

        void NotifyFrontend(int notifyType);

    private:
        /* current app state */
        unsigned long state_ = 0;

        /* Component: Command Queue */
        std::unique_ptr<CommandQueue> command_queue_;

        std::shared_ptr<CoreRuntimeContext> core_runtime_context_;
        std::shared_ptr<GameRuntimeContext> game_runtime_context_;
        std::shared_ptr<class Environment> environment_;
        std::shared_ptr<class Core> core_;

        std::shared_ptr<class VideoContext> video_;
        std::shared_ptr<class InputContext> input_;
        std::shared_ptr<class AudioContext> audio_;

        SpeedLimiter speed_limiter_;

        pid_t emu_thread_id_ = 0;
        FrontendNotifyCallback frontend_notify_ = nullptr;


#ifdef ANDROID
        JNIEnv *thread_jni_env_ = nullptr;
#endif
        AppWindow app_window_{};
    };

}


#endif
