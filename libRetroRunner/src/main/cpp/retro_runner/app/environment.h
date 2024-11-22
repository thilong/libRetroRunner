//
// Created by aidoo on 2024/11/1.
//

#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include <map>
#include <unordered_map>

#include <libretro-common/include/libretro.h>
#include "../types/variable.h"

namespace libRetroRunner {

    class Environment {
        friend class AppContext;

    public:
        Environment();

        ~Environment();

        bool HandleCoreCallback(unsigned int cmd, void *data);

        void UpdateVariable(const std::string &key, const std::string &value, bool notifyCore = false);


        static uintptr_t CoreCallbackGetCurrentFrameBuffer();

        static bool CoreCallbackSetRumbleState(unsigned port, enum retro_rumble_effect effect, uint16_t strength);

        static void CoreCallbackLog(enum retro_log_level level, const char *fmt, ...);

        static void CoreCallbackNotifyAudioState(bool active, unsigned occupancy, bool underrun_likely);

        static retro_proc_address_t CoreCallbackGetProcAddress(const char *sym);

    private:
        bool cmdSetPixelFormat(void *data);

        bool cmdSetHardwareRender(void *data);

        bool cmdGetVariable(void *data);

        bool cmdSetVariables(void *data);

        bool cmdSetVariable(void *data);

        void cmdSetControllers(void *data);

        bool cmdSetSystemAudioVideoInfo(void *data);

        bool cmdSetGeometry(void *data);

        bool cmdGetCurrentFrameBuffer(void *data);

    public:

        const std::string GetVariable(const std::string &key, const std::string &defaultValue = "");

        inline void SetFastForwardSpeed(double speed) { fastForwardSpeed = speed; }

        inline int GetFastForwardFps() { return (int) (gameFps * fastForwardSpeed); }

        inline double GetFps() { return gameFps; }

        inline double GetSampleRate() { return gameSampleRate; }

        inline double GetAspectRatio() { return gameGeometryAspectRatio; }

        inline unsigned int GetGameWidth() { return gameGeometryWidth; }

        inline unsigned int GetGameHeight() { return gameGeometryHeight; }

        inline bool GetRenderUseHWAcceleration() { return renderUseHWAcceleration; }

        inline bool GetRenderUseDepth() { return renderUseDepth; }

        inline bool GetRenderUseStencil() { return renderUseStencil; }

        inline int GetCorePixelFormat() { return core_pixel_format_; }

        inline std::map<int, std::string> &GetSupportControllers() { return supportControllers; }

        void InvokeRenderContextDestroy();

        void InvokeRenderContextReset();

    private:
        std::unordered_map<std::string, struct Variable> variables;

        //核心支持的手柄，由核心返回给前端的,使用retro_set_controller_port_device进行设置
        std::map<int, std::string> supportControllers;
        bool variablesChanged = false;

        unsigned int language = RETRO_LANGUAGE_ENGLISH;
        bool audioEnabled = true;
        bool videoEnabled = true;
        bool fastForwarding = false;
        unsigned int maxUserCount = 4;

        bool coreSupportNoGame = true;

        int core_pixel_format_ = RETRO_PIXEL_FORMAT_XRGB8888;

        int renderContextType = -1;
        unsigned int renderMajorVersion = 0;
        unsigned int renderMinorVersion = 0;

        bool renderUseHWAcceleration = false;
        bool renderUseDepth = false;
        bool renderUseStencil = false;

        retro_hw_context_reset_t renderContextReset = nullptr;
        retro_hw_context_reset_t renderContextDestroy = nullptr;

        unsigned int gameGeometryMaxHeight = 0;
        unsigned int gameGeometryMaxWidth = 0;
        unsigned int gameGeometryHeight = 0;
        unsigned int gameGeometryWidth = 0;
        float gameGeometryAspectRatio;

        double gameFps;
        double gameSampleRate;

        double fastForwardSpeed = 1.0;    //快进速度  1。0为正常速度
        retro_disk_control_callback *diskControllerCallback;
    };

}

#endif
