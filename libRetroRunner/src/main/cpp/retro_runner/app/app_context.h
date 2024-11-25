//
// Created by aidoo on 2024/10/31.
//

#ifndef _APP_H
#define _APP_H

#include <string>
#include "../types/app_command.hpp"
#include "fps_time_throne.hpp"

namespace libRetroRunner {

    class AppContext {

    public:
        AppContext();

        ~AppContext();

    public:
        static std::shared_ptr<AppContext> &CreateInstance();

        static std::shared_ptr<AppContext> &Current();

        void ThreadLoop();

        const std::shared_ptr<class Environment> &GetEnvironment() const;

        const std::shared_ptr<class Paths> &GetPaths() const;

        const std::shared_ptr<class VideoContext> &GetVideo() const;

        const std::shared_ptr<class InputContext> &GetInput() const;

        const std::shared_ptr<class AudioContext> &GetAudio() const;

    public:

        /**
         * Set the appropriate paths for retro runner
         * @param rom       game file
         * @param core      core file
         * @param system    system folder for core
         * @param save      folder to save game states, ram, screenshots
         */
        void SetPaths(const std::string &rom, const std::string &core, const std::string &system, const std::string &save);

        void Start();

        void Pause();

        void Resume();

        void Reset();

        void Stop();

        void SetVideoSurface(int argc, void **argv);

    public:
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
        int sendCommandWithPath(std::string path, int command, bool wait_for_result = false);

        void processCommand();

        void commandLoadCore();

        void commandLoadContent();

        void commandSaveSRAM(std::shared_ptr<Command> &command);

        void commandLoadSRAM(std::shared_ptr<Command> &command);

        void commandSaveState(std::shared_ptr<Command> &command);

        void commandLoadState(std::shared_ptr<Command> &command);

    private:
        /* current app state */
        unsigned long state = 0;

        /* Component: Paths */
        std::shared_ptr<class Paths> paths_ = nullptr;

        /* Component: Command Queue */
        std::unique_ptr<CommandQueue> command_queue_;

        std::shared_ptr<class Core> core_ = nullptr;
        std::shared_ptr<class Environment> environment_;
        std::shared_ptr<class VideoContext> video_;
        std::shared_ptr<class InputContext> input_;
        std::shared_ptr<class AudioContext> audio_;

        FpsTimeThrone fps_time_throne_;

        pid_t emu_thread_id_ = 0;
    };

}


#endif
