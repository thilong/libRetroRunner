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

        void AddCommand(int command);

        void AddCommand(std::shared_ptr<Command> &command);

    public:
        void SetController(unsigned port, int retro_device);

    private:

        void processCommand();

        void commandLoadCore();

        void commandLoadContent();

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
    };

}


#endif
