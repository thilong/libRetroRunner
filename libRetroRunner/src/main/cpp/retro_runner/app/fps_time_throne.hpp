//
// Created by aidoo on 2024/11/14.
//

#ifndef _FPS_TIME_THRONE_HPP
#define _FPS_TIME_THRONE_HPP

#include <chrono>
#include <unistd.h>
namespace libRetroRunner {
    class FpsTimeThrone {
    public:
        FpsTimeThrone() {
            preCheckPointTime = std::chrono::high_resolution_clock::now();
        }

        void Reset() {
            preCheckPointTime = std::chrono::high_resolution_clock::now();
        }

        inline int64_t ElapsedMicro() {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(now - preCheckPointTime).count();
        }

        inline int64_t ElapsedMilli() {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - preCheckPointTime).count();
        }

        inline int64_t ElapsedNano() {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::nanoseconds>(now - preCheckPointTime).count();
        }

        inline long GetFrameTime() { return frameTime; }

        inline int GetFps() { return fps; }

        inline void SetFps(int newFps) {
            this->fps = newFps;
            frameTime = 1000000000 / fps;
        }

        void checkFpsAndWait(int fpsToCheck) {
            if (fpsToCheck != fps) {
                SetFps(fpsToCheck);
            }
            long elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - preCheckPointTime).count();
            if (elapsed < frameTime) {
                usleep((frameTime - elapsed) / 1000);
            }
            preCheckPointTime = std::chrono::high_resolution_clock::now();
        }

    private:
        int fps = 60;
        long frameTime = 1000000000 / fps;  // in nano seconds
        std::chrono::time_point<std::chrono::high_resolution_clock> preCheckPointTime;
    };
}
#endif
