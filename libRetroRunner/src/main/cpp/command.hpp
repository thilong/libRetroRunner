//
// Created by aidoo on 2024/11/13.
//

#ifndef _COMMAND_H
#define _COMMAND_H


#include <memory>
#include <mutex>
#include <queue>

#include <condition_variable>
#include "rr_types.h"

namespace libRetroRunner {
    class Semaphore {
    public:
        explicit Semaphore(int count = 0) : count_(count) {}

        void Signal() {
            std::unique_lock<std::mutex> lock(mutex_);
            ++count_;
            cv_.notify_one();
        }

        void Wait() {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [=] { return count_ > 0; });
            --count_;
        }

    private:
        std::mutex mutex_;
        std::condition_variable cv_;
        int count_;
    };

    class Command {
    public:
        Command(int cmd) {
            command = cmd;
            id = time(nullptr) + random();
        }

        bool threaded = false;
        int command;
        long id;
    };

    template<typename T>
    class ParamCommand : public Command {
    public:
        ParamCommand(int cmd, T arg1) : Command(cmd) {
            arg = arg1;
        }

        T GetArg() { return arg; }

    private:
        T arg;
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
            threaded = true;
        }

        void Signal() { semaphore.Signal(); }

        void Wait() { semaphore.Wait(); }

        R GetResult() { return result; }

        void SetResult(R r) { result = r; }

    private:
        Semaphore semaphore;
        R result;
    };

    template<typename R, typename T>
    class ThreadParamCommand : public ThreadCommand<R> {
    public:
        ThreadParamCommand(int cmd, T arg1) : ThreadCommand<R>(cmd) {
            arg = arg1;
        }

        T GetArg() { return arg; }

    private:
        T arg;
    };

    class CommandQueue {
    public:
        void Push(std::shared_ptr<Command> &cmd) {
            std::lock_guard<std::mutex> lm(mutex);
            queue.push(cmd);
        }

        std::shared_ptr<Command> Pop() {
            std::lock_guard<std::mutex> lm(mutex);
            if (queue.empty()) return nullptr;
            std::shared_ptr<Command> ret = queue.front();
            queue.pop();
            return ret;
        }

        bool Empty() {
            std::lock_guard<std::mutex> lm(mutex);
            return queue.empty();
        }

        int Size() {
            std::lock_guard<std::mutex> lm(mutex);
            return queue.size();
        }

    private:
        mutable std::mutex mutex;
        std::queue<std::shared_ptr<Command>> queue;
    };

}


#endif
