//
// Created by aidoo on 2024/11/14.
//

#ifndef _FPS_TIME_THRONE_HPP
#define _FPS_TIME_THRONE_HPP

#include <chrono>
#include <unistd.h>

namespace libRetroRunner {
    class SpeedLimiter {
    public:
        SpeedLimiter() {
            check_point_ = std::chrono::high_resolution_clock::now();
        }

        void Reset() {
            check_point_ = std::chrono::high_resolution_clock::now();
        }

        inline int64_t ElapsedMicro() {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(now - check_point_).count();
        }

        inline int64_t ElapsedMilli() {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - check_point_).count();
        }

        inline int64_t ElapsedNano() {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::nanoseconds>(now - check_point_).count();
        }

        inline void SetMultiple(double multiple) {
            multiple_ = multiple;
        }

        void CheckAndWait(int fpsToCheck) {
            long frame_time = 1000000000L / (long) (fpsToCheck * multiple_);
            long elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - check_point_).count();
            if (elapsed < frame_time) {
                usleep((frame_time - elapsed) / 1000);
            }
            check_point_ = std::chrono::high_resolution_clock::now();
        }

    private:
        double multiple_ = 1.0;
        std::chrono::time_point<std::chrono::high_resolution_clock> check_point_;
    };
}
#endif
