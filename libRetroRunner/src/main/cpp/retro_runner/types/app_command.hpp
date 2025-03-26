//
// Created by Aidoo.TK on 2024/11/21.
//

#ifndef _APP_COMMAND_H
#define _APP_COMMAND_H

#include <time.h>
#include <random>
#include <memory>
#include <queue>

#include "semaphore_rr.h"

namespace libRetroRunner {

    enum AppCommands {
        kNone = 10,
        kInitApp,
        kLoadCore,
        kLoadContent,
        kInitComponents,
        kLoadVideo,
        kUnloadVideo,

        kResetGame,
        kPauseGame,
        kStopGame,

        kEnableAudio,
        kDisableAudio,

        kTakeScreenshot,

        //---here
        kSaveSRAM,
        kLoadSRAM,

        kSaveState,
        kLoadState,


        kLoadCheats,
        kSaveCheats,
        kSaveCheatsAsync
    };

    enum CommandType {
        kNormalCommand = 1,
        kParamCommand = 2,
        kThreadCommand = 3
    };

    class Command {
    public:
        explicit Command(int cmd) {
            command_type_ = CommandType::kNormalCommand;
            command_ = cmd;
            id_ = time(nullptr) + random();
        }

        virtual ~Command() = default;

        inline int GetCommand() const { return command_; }

        inline int GetCommandType() const { return command_type_; };

        inline long GetId() const { return id_; }

    protected:
        int command_type_;
        int command_;
        long id_;
    };

    template<typename T>
    class ParamCommand : public Command {
    public:
        ParamCommand(int cmd, T arg1) : Command(cmd) {
            arg_ = arg1;
            command_type_ = CommandType::kParamCommand;
        }

        T GetArg() { return arg_; }

    private:
        T arg_;
    };

    template<typename R, typename T>
    class ThreadCommand : public ParamCommand<T> {
    public:
        ThreadCommand(int cmd, T arg1) : ParamCommand<T>(cmd, arg1) {
            Command::command_type_ = CommandType::kThreadCommand;
            should_wait_complete_ = true;

        }

        R GetResult() { return result_; }

        void SetWaitComplete(bool flag) { should_wait_complete_ = flag; }

        void SetResult(R value) { result_ = value; }

        void Signal() {
            if (should_wait_complete_)
                sem_.Signal();
        }

        void Wait() {
            if (should_wait_complete_)
                sem_.Wait();
        }

    private:
        RRSemaphore sem_;
        bool should_wait_complete_;
        R result_;
    };

    class CommandQueue {
    public:
        void Push(std::shared_ptr<Command> &cmd) {
            std::lock_guard<std::mutex> lm(mutex_);
            queue_.push(cmd);
        }

        std::shared_ptr<Command> Pop() {
            std::lock_guard<std::mutex> lm(mutex_);
            if (queue_.empty()) return nullptr;
            std::shared_ptr<Command> ret = queue_.front();
            queue_.pop();
            return ret;
        }

    private:
        mutable std::mutex mutex_;
        std::queue<std::shared_ptr<Command>> queue_;
    };

}

#endif
