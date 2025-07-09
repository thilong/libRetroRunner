//
// Created by Aidoo.TK on 2024/11/1.
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

        inline void SetGameRuntimeContext(const std::shared_ptr<class GameRuntimeContext>& game_runtime_context) {
            game_runtime_context_ = game_runtime_context;
        }

        inline void SetCoreRuntimeContext(const std::shared_ptr<class CoreRuntimeContext>& core_runtime_context) {
            core_runtime_context_ = core_runtime_context;
        }

        const std::string GetVariable(const std::string &key, const std::string &defaultValue = "");

        inline void SetAppSandBoxPath(const std::string &path) {
            appSandBoxPath_ = path;
        }
        inline std::string& GetAppSandBoxPath() { return appSandBoxPath_; }
    private:
        std::string appSandBoxPath_;
        bool variablesChanged = false;
        std::unordered_map<std::string, struct Variable> variables;

        std::weak_ptr<class GameRuntimeContext> game_runtime_context_;
        std::weak_ptr<class CoreRuntimeContext> core_runtime_context_;

        bool audioEnabled = true;
        bool videoEnabled = true;

        retro_disk_control_callback *diskControllerCallback;
    };

}

#endif
