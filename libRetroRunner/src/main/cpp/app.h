//
// Created by aidoo on 2024/10/31.
//

#ifndef _APP_H
#define _APP_H

#include <string>
#include "rr_types.h"
#include "core.h"
#include "environment.h"
#include "video_context.h"
#include "input_context.h"
#include "audio_context.h"

#include "utils/threadsafe_queue.hpp"

namespace libRetroRunner {

    enum AppState {
        kNone = 0,
        kPathSet = 1 << 0,    //1
        kCoreReady = 1 << 1,  //2
        kContentReady = 1 << 2,  //4
        kVideoReady = 1 << 3, //8
        kRunning = 1 << 4,   //16
        kPaused = 1 << 5,
    };

    class AppContext {


    public:
        AppContext();

        ~AppContext();

        /*模拟器生命周期相关 --------------------- */
        void SetFiles(const std::string &romPath, const std::string &corePath, const std::string &systemPath,
                      const std::string &savePath);

        void SetVariable(const std::string &key, const std::string &value, bool notifyCore = false);

        void SetVideoRenderTarget(void **args, int argc);

        void SetVideoRenderSize(unsigned width, unsigned height);

        void AddCommand(int command);

        void Start();

        void Pause();

        void Resume();

        void Reset();

        void Stop();

        void ThreadLoop();

    public:
        /*模拟器运行功能相关 --------------------- */
        /**
         * 设置玩家控制器，以方便核心进行不同的事件调用
         * @param port 玩家位置
         * @param device 设备类型
         * @see RETRO_DEVICE_JOYPAD , RETRO_DEVICE_ANALOG
         */
        void SetController(int port, int device);

    public:
        static AppContext *CreateInstance();

        static AppContext *Current() {
            return instance.get();
        };

        /**
         * 获取游戏相关的环境变量
         * @return
         */
        inline Environment *GetEnvironment() { return environment.get(); }

        inline VideoContext *GetVideo() { return video.get(); }

        inline InputContext *GetInput() { return input.get(); }

        inline AudioContext *GetAudio() { return audio.get(); }



        inline int GetState() { return state; }

    private: //以下函数只在模拟线程中调用
        void processCommand();

        void cmdLoadCore();

        void cmdLoadContent();

        void cmdInitVideo();

        void cmdUnloadVideo();

        void cmdInitInput();

        void cmdInitAudio();

    private:

        std::string rom_path = "";
        std::string core_path = "";
        std::string system_path = "";
        std::string save_path = "";

        int state = 0;
        threadsafe_queue<int> commands;   //等待处理的命令

        std::unique_ptr<Core> core = nullptr;
        std::unique_ptr<Environment> environment = nullptr;
        std::unique_ptr<VideoContext> video = nullptr;
        std::unique_ptr<InputContext> input = nullptr;
        std::unique_ptr<AudioContext> audio = nullptr;


        static std::unique_ptr<AppContext> instance;

        friend class RetroCallbacks;
    };

    class RetroCallbacks {

    public:
        static void libretro_callback_hw_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch);

        static bool libretro_callback_set_environment(unsigned cmd, void *data);

        static void libretro_callback_audio_sample(int16_t left, int16_t right);

        static size_t libretro_callback_audio_sample_batch(const int16_t *data, size_t frames);

        static void libretro_callback_input_poll(void);

        static int16_t libretro_callback_input_state(unsigned port, unsigned device, unsigned index, unsigned id);
    };

}


#endif
