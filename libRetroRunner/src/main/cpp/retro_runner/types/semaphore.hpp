//
// Created by aidoo on 2024/11/21.
//

#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include <mutex>

namespace libRetroRunner {
    class Semaphore {
    public:
        explicit Semaphore(int count = 0) : _count(count) {}

        void Signal() {
            std::unique_lock<std::mutex> lock(_mutex);
            ++_count;
            _cv.notify_one();
        }

        void Wait() {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [=] { return _count > 0; });
            --_count;
        }

    private:
        std::mutex _mutex;
        std::condition_variable _cv;
        int _count;
    };
}
#endif
