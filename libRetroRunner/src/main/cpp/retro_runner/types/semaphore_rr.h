//
// Created by aidoo on 2024/11/21.
//

#ifndef _SEMAPHORE_RR_H
#define _SEMAPHORE_RR_H

#include <mutex>

namespace libRetroRunner {
    class RRSemaphore {
    public:
        explicit RRSemaphore(int count = 0);
        void Signal() ;
        void Wait();

    private:
        std::mutex mutex_;
        std::condition_variable cv_;
        int count_;
    };
}
#endif
