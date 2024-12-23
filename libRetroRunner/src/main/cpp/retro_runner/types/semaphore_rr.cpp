//
// Created by Aidoo.TK on 2024/11/21.
//
#include "semaphore_rr.h"

namespace libRetroRunner {
    RRSemaphore::RRSemaphore(int count) : count_(count) {

    }

    void RRSemaphore::Signal(){
        std::unique_lock<std::mutex> lock(mutex_);
        ++count_;
        cv_.notify_one();
    }

    void RRSemaphore::Wait()  {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [=] { return count_ > 0; });
        --count_;
    }
}