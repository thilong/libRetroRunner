//
// Created by aidoo on 2024/11/21.
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
        kLoadCore,
        kLoadContent,
        kInitVideo,
        kInitInput,
        kInitAudio,

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
        kSaveStateAsync,
        kLoadState,
        kLoadStateAsync,


        kLoadCheats,
        kSaveCheats,
        kSaveCheatsAsync
    };

    class Command {
    public:
        Command(int cmd) {
            command_ = cmd;
            id_ = time(nullptr) + random();
        }

        virtual ~Command() {}

        /**
        * signal command finish, only in multi-threaded and threaded = true
        */
        virtual void Signal() {}

        /**
        * wait for command finish, only in multi-threaded and threaded = true
        */
        virtual void Wait() {}

        inline bool ShouldWaitComplete() { return wait_complete_; }

        inline void SetWaitComplete(bool flag) { wait_complete_ = flag; }

        inline int GetCommand() { return command_; }

        inline long GetId() { return id_; }

    protected:
        bool wait_complete_ = false;
        int command_;
        long id_;
    };

    template<typename R, typename T>
    class ThreadCommand : public Command {
    public:
        ThreadCommand(int cmd, T arg1) : Command(cmd) {
            wait_complete_ = true;
            arg_ = arg1;
        }

        T GetArg() { return arg_; }

        R GetResult() { return result_; }

        void SetResult(R value) { result_ = value; }

        void Signal() override {
            if (wait_complete_)
                sem_.Signal();
        }

        void Wait() override {
            if (wait_complete_)
                sem_.Wait();
        }

    private:
        RRSemaphore sem_;
        T arg_;
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

        bool Empty() {
            std::lock_guard<std::mutex> lm(mutex_);
            return queue_.empty();
        }

        int Size() {
            std::lock_guard<std::mutex> lm(mutex_);
            return queue_.size();
        }

    private:
        mutable std::mutex mutex_;
        std::queue<std::shared_ptr<Command>> queue_;
    };

}

#endif
