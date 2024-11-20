//
// Created by aidoo on 2024/11/13.
//

#ifndef _PREF_COUNTER_H
#define _PREF_COUNTER_H

#include <chrono>

namespace libRetroRunner {
    class PrefCounter {
    public:
        PrefCounter() {
            preCheckPointTime = std::chrono::high_resolution_clock::now();
        }

        void Reset() {
            preCheckPointTime = std::chrono::high_resolution_clock::now();
        }

        int64_t ElapsedMicro() {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(now - preCheckPointTime).count();
        }

        int64_t ElapsedMilli() {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - preCheckPointTime).count();
        }

        int64_t ElapsedNano() {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::nanoseconds>(now - preCheckPointTime).count();
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> preCheckPointTime;
    };
}
#endif
