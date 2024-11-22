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


        kSaveSRAM,
        kLoadSRAM,

        kSaveState,
        kSaveStateAsync,
        kLoadState,
        kLoadStateAsync,

        kTakeScreenshot,

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
        * 发送信号，表示命令已经执行完毕，只在多线程中使用
        */
        virtual void Signal() {}

        /**
        * 等待命令执行完毕，只在多线程中使用
        */
        virtual void Wait() {}

        inline bool IsThreaded() { return threaded_; }

        inline int GetCommand() { return command_; }

        inline long GetId() { return id_; }

    protected:
        bool threaded_ = false;
        int command_;
        long id_;
    };

    /**
     * 带有1个参数的命令
     * @tparam T
     */
    template<typename T>
    class ParamCommand : public Command {
    public:
        ParamCommand(int cmd, T arg1) : Command(cmd) {
            arg_ = arg1;
        }

        T GetArg() { return arg_; }

    private:
        T arg_;
    };

    /**
     * 多线程所使用的命令，用于线程间任务执行
     * 不能在同一线程中使用这个类型，否则无法正确运行
     * @tparam R 返回值类型， 一般int, 0表示成功，其他值表示错误
     */
    template<typename R>
    class ThreadCommand : public Command {
    public:
        ThreadCommand(int cmd) : Command(cmd) {
            threaded_ = true;
        }

        void Signal() { semaphore_.Signal(); }

        void Wait() { semaphore_.Wait(); }

        R GetResult() { return result_; }

        void SetResult(R r) { result_ = r; }

    private:

        RRSemaphore semaphore_;
        R result_;
    };

    template<typename R, typename T>
    class ThreadParamCommand : public ThreadCommand<R> {
    public:
        ThreadParamCommand(int cmd, T arg1) : ThreadCommand<R>(cmd) {
            arg_ = arg1;
        }

        T GetArg() { return arg_; }

    private:
        T arg_;
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
