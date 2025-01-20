//
// Created by Aidoo.TK on 2024/11/1.
//
#include "environment.h"
#include <retro_runner/runtime_contexts/game_context.h>
#include <retro_runner/runtime_contexts/core_context.h>

#include <EGL/egl.h>
#include "../types/log.h"
#include "../types/retro_types.h"

#include "setting.h"
#include "../video/video_context.h"
#include "app_context.h"
#include "paths.h"
#include "../vfs/vfs_context.h"

#define POINTER_VAL(_TYPE_) (*((_TYPE_*)data))

#define LOGD_Env(...) LOGD("[Environment] "  __VA_ARGS__)
#define LOGW_Env(...) LOGW("[Environment] "  __VA_ARGS__)

//变量控制相关
namespace libRetroRunner {

    void Environment::UpdateVariable(const std::string &key, const std::string &value, bool notifyCore) {

    }

    Environment::Environment() {
        diskControllerCallback = nullptr;
    }

    Environment::~Environment() = default;
}

namespace libRetroRunner {
    bool Environment::HandleCoreCallback(unsigned int cmd, void *data) {
        switch (cmd) {
            case RETRO_ENVIRONMENT_SET_ROTATION: {
                auto newRotation = *(const unsigned *) data;
                auto gameCtx = AppContext::Current()->GetGameRuntimeContext();
                gameCtx->SetGeometryRotation(newRotation);
                gameCtx->SetGeometryChanged(true);
                AppContext::Current()->NotifyFrontend(AppNotifications::kAppNotificationGameGeometryChanged);
                LOGD_Env("call RETRO_ENVIRONMENT_SET_ROTATION -> [%u]", newRotation);
                break;
            }
            case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_CAN_DUPE -> false");
                POINTER_VAL(bool) = true;
                return true;
            }
            case RETRO_ENVIRONMENT_SET_MESSAGE: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_MESSAGE -> 1");
                auto *msg = static_cast<struct retro_message *>(data);
                LOGD("Message: %s", msg->msg);
                return true;
            }
            case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
                auto core_runtime = core_runtime_context_.lock();
                if (core_runtime) {
                    std::string systemPath = core_runtime->GetSystemPath();
                    if (!systemPath.empty()) {
                        POINTER_VAL(const char*) = systemPath.c_str();
                        LOGD_Env("call RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY -> %s",
                                 systemPath.c_str());
                        return true;
                    }
                }
                LOGD_Env("call RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY -> [empty]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
                return cmdSetPixelFormat(data);
            }
            case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE -> disk control");
                diskControllerCallback = static_cast<retro_disk_control_callback *>(data);
                return true;
            }
            case RETRO_ENVIRONMENT_SET_HW_RENDER:
            case RETRO_ENVIRONMENT_SET_HW_RENDER | RETRO_ENVIRONMENT_EXPERIMENTAL: {
                return cmdSetHardwareRender(data);
            }
            case RETRO_ENVIRONMENT_GET_VARIABLE: {
                //LOGD_Env("call RETRO_ENVIRONMENT_GET_VARIABLE");
                return cmdGetVariable(data);
            }
            case RETRO_ENVIRONMENT_SET_VARIABLES: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_VARIABLES");
                return cmdSetVariables(data);
            }
            case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
                //LOGD_Env("call RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE");
                POINTER_VAL(bool) = variablesChanged;
                variablesChanged = false;
                return true;
            }
            case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME -> record");
                core_runtime_context_.lock()->SetSupportNoGame(POINTER_VAL(bool));
                return true;
            }
            case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK  -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK -> [NO IMPL]");
                //auto callback = static_cast<const struct retro_audio_callback *>(data);
                return false;
            }
            case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE");
                auto callback = static_cast<struct retro_rumble_interface *>(data);
                callback->set_rumble_state = &Environment::CoreCallbackSetRumbleState;
                return false;
            }
            case RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES");
                POINTER_VAL(uint64_t) = (1 << RETRO_DEVICE_JOYPAD) | (1 << RETRO_DEVICE_ANALOG) |
                                        (1 << RETRO_DEVICE_POINTER);
                return false;
            }
            case RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_LOG_INTERFACE");
                auto callback = static_cast<struct retro_log_callback *>(data);
                callback->log = &Environment::CoreCallbackLog;
                return true;
            }
            case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_PERF_INTERFACE -> [NO IMPL]");
                //TODO:在这里添加性能计数器
                return false;
            }
            case RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY , RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY -> [NO IMPL]");
                //TODO:在这里返回内容目录
                return false;
            }
            case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: {
                auto game_runtime = game_runtime_context_.lock();
                if (game_runtime) {
                    std::string path = game_runtime->GetSavePath();
                    if (!path.empty()) {
                        POINTER_VAL(const char*) = path.c_str();
                        LOGD_Env("call RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY -> %s", path.c_str());
                        return true;
                    }
                }
                LOGD_Env("call RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY -> [empty]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: {
                //用于通知前端视频与音频参数发生变化，在可能的情况下，前端可以重新初始化视频与音频上下文 ，
                //这个回调不能用于通知游戏画面大小变化。，应当使用RETRO_ENVIRONMENT_SET_GEOMETRY
                return cmdSetSystemAudioVideoInfo(data);
            }
            case RETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO: {
                cmdSetControllers(data);
                return true;
            }
            case RETRO_ENVIRONMENT_SET_MEMORY_MAPS: {
                //TODO:通知前端核心所使用的内存空间
                LOGD_Env("call RETRO_ENVIRONMENT_SET_MEMORY_MAPS -> [NO IMPL]");
                const struct retro_memory_map *map = static_cast<const struct retro_memory_map *>(data);
                return false;
            }
            case RETRO_ENVIRONMENT_SET_GEOMETRY: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_GEOMETRY");
                //通知游戏画面内容大小发生变化。 不能在这个回调中改变渲染上下文环境
                return cmdSetGeometry(data);
            }
            case RETRO_ENVIRONMENT_GET_USERNAME: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_USERNAME -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_GET_LANGUAGE: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_LANGUAGE -> en");
                POINTER_VAL(unsigned) = core_runtime_context_.lock()->GetLanguage();
                return true;
            }
            case RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER");
                return cmdGetCurrentFrameBuffer(data);
            }
            case RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE: {
                //返回前端硬件渲染的类型，不是所有核心都需要这个回调
                LOGD_Env("call RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE -> [NO IMPL]");
                //auto callback = static_cast<const struct retro_hw_render_interface **>(data);
                return false;
            }
            case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS: {
                //通知前端核心是否支持成就
                LOGD_Env("call RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS -> [NO IMPL]");
                return true;
            }
            case RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE: {
                //通知前端核心是否支持硬件渲染上下文协商
                LOGD_Env(
                        "call RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS: {
                //通知前端核心是否支持序列化特性
                LOGD_Env("call RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT: {
                //通知前端:核心是否支持共享硬件渲染上下文
                LOGD_Env("call RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_GET_VFS_INTERFACE: {
                //TODO:获取虚拟文件系统
                LOGD_Env("call RETRO_ENVIRONMENT_GET_VFS_INTERFACE -> [Doing nothing]");
                struct retro_vfs_interface_info *vfs = static_cast<struct retro_vfs_interface_info *>(data);
                vfs->iface = &VirtualFileSystemContext::vfsInterface;
                return true;
            }
            case RETRO_ENVIRONMENT_GET_LED_INTERFACE: {
                //TODO:获取LED系统
                LOGD_Env("call RETRO_ENVIRONMENT_GET_LED_INTERFACE -> [NO IMPL]");
                return false;

            }
            case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE");
                int ret = 0;
                if (audioEnabled)
                    ret = ret | RETRO_AV_ENABLE_VIDEO;
                if (videoEnabled)
                    ret = ret | RETRO_AV_ENABLE_AUDIO;
                POINTER_VAL(retro_av_enable_flags) = (retro_av_enable_flags) ret;
                return true;
            }
            case RETRO_ENVIRONMENT_GET_MIDI_INTERFACE: {
                //TODO:获取MIDI系统
                LOGD_Env("call RETRO_ENVIRONMENT_GET_MIDI_INTERFACE -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_GET_FASTFORWARDING: {
                //LOGD_Env("call RETRO_ENVIRONMENT_GET_FASTFORWARDING");
                auto game_ctx = game_runtime_context_.lock();
                POINTER_VAL(bool) = game_ctx->GetIsFastForwarding();
                return true;
            }
            case RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE: {
                //返回目标刷新率
                LOGD_Env("call RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE");
                POINTER_VAL(float) = 60.0f;
                return false;
            }
            case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: {
                //TODO:返回前端是否支持以掩码的方式一次性获取所有的输入信息,如果返回true, 则需要在retro_input_state_t方法中检测RETRO_DEVICE_ID_JOYPAD_MASK并返回所有的输入
                LOGD_Env("call RETRO_ENVIRONMENT_GET_INPUT_BITMASKS");
                return true;
            }
            case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: {
                //TODO:返回前端所支持的核心选项版本, 0, 1, 2, 不同的版本会有不同的核心选项组织方式
                LOGD_Env("call RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION");
                POINTER_VAL(unsigned) = 0;
                return true;
            }
            case RETRO_ENVIRONMENT_SET_CORE_OPTIONS: {
                /*TODO:通知前端核心选项，已经被当前版本的核心所弃用。应当使用 RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2
                    这个回调是为了用于取代 RETRO_ENVIRONMENT_SET_VARIABLES (RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION 返回 >= 1时)，
                    如果核心使用了新的版本返回选项，则需要实现这个回调, 其结构体为retro_core_option_definition，类似于json的实现
                 */
                LOGD_Env("call RETRO_ENVIRONMENT_SET_CORE_OPTIONS -> [NO IMPL]");
                //auto request = static_cast<const struct retro_core_option_definition *>(data);
                return false;
            }
            case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL: {
                /*TODO:RETRO_ENVIRONMENT_SET_CORE_OPTIONS的变体，用于支持多语言*/
                LOGD_Env("call RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL -> [NO IMPL]");
                //auto request = static_cast<const struct retro_core_options_intl *>(data);
                return false;

            }
            case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY: {
                //用于控制核心选项的可见性
                //LOGD_Env("call RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY -> [NO IMPL]");
                //auto request = static_cast<const struct retro_core_option_display *>(data);
                return true;
            }
            case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER: {
                //返回前端所期望的硬件渲染类型
                LOGD_Env("call RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER");
                POINTER_VAL(retro_hw_context_type) = RETRO_HW_CONTEXT_OPENGL;
                return true;
            }
            case RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION: {
                //返回前端所支持的磁盘控制接口版本, 如果值 >= 1, 核心会使用 RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE
                LOGD_Env("call RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION");
                POINTER_VAL(unsigned) = 0;
                return true;
            }
            case RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE: {
                //通知前端核心所支持的磁盘控制扩展接口
                LOGD_Env("call RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE -> [NO IMPL]");
                //auto request = static_cast<const struct retro_disk_control_ext_interface *>(data);
                return false;
            }
            case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION: {
                //返回前端所支持的消息接口版本, 0表示只支持RETRO_ENVIRONMENT_SET_MESSAGE, 1表示还支持RETRO_ENVIRONMENT_SET_MESSAGE_EXT
                LOGD_Env("call RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION");
                POINTER_VAL(unsigned) = 0;
                return true;
            }
            case RETRO_ENVIRONMENT_SET_MESSAGE_EXT: {
                //向前端发送一个用户需要关心的信息，其他消息使用日志接口来返回
                LOGD_Env("call RETRO_ENVIRONMENT_SET_MESSAGE_EXT");
                auto request = static_cast<const struct retro_message_ext *>(data);
                LOGW("Important: %s", request->msg);
                return true;
            }
            case RETRO_ENVIRONMENT_GET_INPUT_MAX_USERS: {
                //返回前端所支持的最大用户数
                LOGD_Env("call RETRO_ENVIRONMENT_GET_INPUT_MAX_USERS");
                POINTER_VAL(unsigned) = Setting::Current()->GetMaxPlayerCount();
                return true;
            }
            case RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK: {
                //向核心注册一个回调，用于核心通知前端音频缓冲区的状态，比如有时核心需要跳过一些音频帧
                LOGD_Env("call RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK");
                auto request = static_cast<struct retro_audio_buffer_status_callback *>(data);
                request->callback = &Environment::CoreCallbackNotifyAudioState;
                return false;
            }
            case RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY: {
                //通知前端核心所需要的最小音频延迟
                LOGD_Env("call RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE: {
                //通知前端核心是否应该快进, 比如有时核心需要跳过一些帧时
                LOGD_Env("call RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE -> [NO IMPL]");
                //auto request = static_cast<const struct retro_fastforwarding_override *>(data);
                return false;
            }
            case RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_GAME_INFO_EXT -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2: {
                //TODO:通知前端核心选项，用于替代 RETRO_ENVIRONMENT_SET_VARIABLES， 只在RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION返回 >= 2时使用
                LOGD_Env("call RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2 -> [NO IMPL]");
                //auto request = static_cast<const struct retro_core_options_v2 *>(data);
                return false;
            }
            case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL: {
                //TODO:通知前端核心选项，用于替代 RETRO_ENVIRONMENT_SET_VARIABLES， 只在RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION返回 >= 2时使用
                //RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2 的变体，支持多语言
                LOGD_Env("call RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL -> [NO IMPL]");
                //auto request = static_cast<const struct retro_core_options_v2 *>(data);
                return false;
            }
            case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK: {
                //用于前端向核心通知哪些核心设置应该显示或者应该隐藏
                LOGD_Env(
                        "call RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_VARIABLE: {
                //核心通知前端选项值发生变化。
                LOGD_Env("call RETRO_ENVIRONMENT_SET_VARIABLE");
                return cmdSetVariable(data);
                return true;
            }
            case RETRO_ENVIRONMENT_GET_THROTTLE_STATE: {
                //用于核心获取前端的帧率运行情況
                LOGD_Env("call RETRO_ENVIRONMENT_GET_THROTTLE_STATE -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_GET_SAVESTATE_CONTEXT: {
                //todo:用于核心获取前端想要的存档状态,在这里控制存档的类型，是用于对战还是正常游戏
                LOGD_Env("call RETRO_ENVIRONMENT_GET_SAVESTATE_CONTEXT");
                POINTER_VAL(retro_savestate_context) = RETRO_SAVESTATE_CONTEXT_NORMAL;
                //auto request = static_cast<retro_savestate_context *>(data);
                return true;
            }
            case RETRO_ENVIRONMENT_GET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_SUPPORT: {
                //在SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE之前调用，用于确认所支持的类型
                LOGD_Env(
                        "call RETRO_ENVIRONMENT_GET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_SUPPORT");
                auto request = static_cast<struct retro_hw_render_context_negotiation_interface *>(data);
                return false;
            }
            case RETRO_ENVIRONMENT_GET_JIT_CAPABLE: {
                //用于确认当前环境是否支持JIT,主要用于iOS, Javascript
                LOGD_Env("call RETRO_ENVIRONMENT_GET_JIT_CAPABLE -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_GET_MICROPHONE_INTERFACE: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_MICROPHONE_INTERFACE -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_GET_DEVICE_POWER: {
                //todo:返回设备的电量，有的核心有可能在低电量下运行效率缓慢。
                LOGD_Env("call RETRO_ENVIRONMENT_GET_DEVICE_POWER -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_NETPACKET_INTERFACE: {
                LOGD_Env("call RETRO_ENVIRONMENT_SET_NETPACKET_INTERFACE -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_GET_PLAYLIST_DIRECTORY: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_PLAYLIST_DIRECTORY -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_GET_FILE_BROWSER_START_DIRECTORY: {
                LOGD_Env("call RETRO_ENVIRONMENT_GET_FILE_BROWSER_START_DIRECTORY -> [NO IMPL]");
                return false;
            }
            case RETRO_ENVIRONMENT_SET_SAVE_STATE_IN_BACKGROUND: {
                //用于通知前端在后台存储存档的状态
                break;
            }
            default:
                LOGD_Env("not handled: %d, %x -> false  -> [NO IMPL]", cmd, cmd);
                break;
        }
        return false;
    }

    bool Environment::cmdSetPixelFormat(void *data) {
        auto core_ctx = core_runtime_context_.lock();
        core_ctx->SetPixelFormat(POINTER_VAL(enum retro_pixel_format));
        LOGD_Env("call RETRO_ENVIRONMENT_SET_PIXEL_FORMAT -> game pixel format : %d",
                 core_ctx->GetPixelFormat());
        return true;
    }

    bool Environment::cmdSetHardwareRender(void *data) {

        if (data == nullptr) return false;
        auto hwRender = static_cast<struct retro_hw_render_callback *>(data);
        auto core_ctx = core_runtime_context_.lock();

        core_ctx->SetRenderMajorVersion((int) hwRender->version_major);
        core_ctx->SetRenderMinorVersion((int) hwRender->version_minor);
        core_ctx->SetRenderContextType(hwRender->context_type);

        core_ctx->SetRenderUseHardwareAcceleration(true);
        core_ctx->SetRenderUseDepth(hwRender->depth);
        core_ctx->SetRenderUseStencil(hwRender->stencil);

        core_ctx->SetRenderHWContextResetCallback(hwRender->context_reset);
        core_ctx->SetRenderHWContextDestroyCallback(hwRender->context_destroy);
        hwRender->get_proc_address = &Environment::CoreCallbackGetProcAddress;
        hwRender->get_current_framebuffer = &Environment::CoreCallbackGetCurrentFrameBuffer;
        return true;
    }

    bool Environment::cmdGetVariable(void *data) {
        auto request = static_cast<struct retro_variable *>(data);
        auto foundVariable = variables.find(std::string(request->key));

        if (foundVariable == variables.end()) {
            LOGD_Env("call RETRO_ENVIRONMENT_GET_VARIABLE: %s -> null", request->key);
            return false;
        }
        request->value = foundVariable->second.value.c_str();
        LOGD_Env("call RETRO_ENVIRONMENT_GET_VARIABLE: %s -> %s", request->key, request->value);
        return true;
    }

    bool Environment::cmdSetVariables(void *data) {
        /*核心通知给前端的有可能的选项值*/
        auto request = static_cast<const struct retro_variable *>(data);
        unsigned idx = 0;
        while (request[idx].key != nullptr) {
            cmdSetVariable((void *) (&request[idx]));
            idx++;
        }
        return true;
    }

    bool Environment::cmdSetVariable(void *data) {
        auto request = static_cast<const struct retro_variable *>(data);
        if (request && request->key != nullptr) {
            std::string key(request->key);
            std::string description(request->value);
            std::string value(request->value);


            auto firstValueStart = value.find(';') + 2;
            auto firstValueEnd = value.find('|', firstValueStart);
            value = value.substr(firstValueStart, firstValueEnd - firstValueStart);

            auto currentVariable = variables[key];
            currentVariable.key = key;
            currentVariable.description = description.substr(0, description.find(';'));
            currentVariable.options = description.substr(description.find(';') + 2);

            if (currentVariable.value.empty()) {
                currentVariable.value = value;
            }

            variables[key] = currentVariable;
            LOGD_Env("core provide variable: %s -> %s: %s", key.c_str(), value.c_str(), description.c_str());
        }
        return true;
    }

    bool Environment::cmdSetSystemAudioVideoInfo(void *data) {
        if (!data) {
            LOGD_Env("call RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO -> no input data");
            return false;
        }
        auto avInfo = static_cast<const struct retro_system_av_info *>(data);

        cmdSetGeometry((void *) &(avInfo->geometry));

        auto game_ctx = game_runtime_context_.lock();
        if (game_ctx) {
            game_ctx->SetSampleRate(avInfo->timing.sample_rate);
            game_ctx->SetFps(avInfo->timing.fps);
        }

        //TODO: 需要把参数同步给app, 以确认是否需要重建音频上下文和运行速度限制
        return true;
    }

    bool Environment::cmdSetGeometry(void *data) {
        auto geometry = static_cast<struct retro_game_geometry *>(data);

        auto game_ctx = game_runtime_context_.lock();

        bool geometry_changed = (geometry->base_height != game_ctx->GetGeometryHeight() ||
                                 geometry->base_width != game_ctx->GetGeometryWidth());

        game_ctx->SetGeometryWidth(geometry->base_width);
        game_ctx->SetGeometryHeight(geometry->base_height);
        game_ctx->SetGeometryMaxWidth(geometry->max_width);
        game_ctx->SetGeometryMaxHeight(geometry->max_height);
        game_ctx->SetGeometryAspectRatio(geometry->aspect_ratio);

        if (geometry_changed) {
            game_ctx->SetGeometryChanged(true);
            AppContext::Current()->NotifyFrontend(AppNotifications::kAppNotificationGameGeometryChanged);

        }
        return true;
    }

    bool Environment::cmdGetCurrentFrameBuffer(void *data) {
        LOGW_Env("call cmdGetCurrentFrameBuffer -> not impl yet");
        /* TODO: 用于返回当前的软件渲染帧缓冲区, 当使用软件渲染时，可用于性能调优
        auto callback = static_cast<struct retro_framebuffer *>(data);
        callback->format = (enum retro_pixel_format) core_pixel_format_;
        */
        return false;
    }

    void Environment::cmdSetControllers(void *data) {
        //通知前端支持的控制器信息，以方便用户选择不同的控制器,然后使用retro_set_controller_port_device进行设置
        LOGD_Env("call RETRO_ENVIRONMENT_SET_CONTROLLER_INFO -> save supported controller infos.");
        auto core_ctx = core_runtime_context_.lock();
        struct retro_controller_info *controller = static_cast<struct retro_controller_info *>(data);
        while (controller != nullptr && controller->types != nullptr) {
            for (int i = 0; i < controller->num_types; ++i) {
                const retro_controller_description controllerDesc = controller->types[i];
                core_ctx->SetSupportController(controllerDesc.id, controllerDesc.desc);
                //LOGD_Env("controller %d: %s, id: %d", i, controllerDesc.desc, controllerDesc.id);
            }
            controller++;
        }
    }

}

//核心回调函数
namespace libRetroRunner {
    uintptr_t Environment::CoreCallbackGetCurrentFrameBuffer() {
        uintptr_t ret = 0;

        auto appContext = AppContext::Current();
        if (appContext) {
            auto video = appContext->GetVideo();
            if (video) {
                ret = (uintptr_t) video->GetCurrentFramebuffer();
            }
        }
        return ret;
    }

    bool Environment::CoreCallbackSetRumbleState(unsigned int port, enum retro_rumble_effect effect, uint16_t strength) {
        return false;
    }

    void Environment::CoreCallbackLog(enum retro_log_level level, const char *fmt, ...) {
        va_list argv;
        va_start(argv, fmt);

        switch (level) {
#if CORE_LOG_DEBUG
            case RETRO_LOG_DEBUG:
                __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG, fmt, argv);
                break;
#endif
            case RETRO_LOG_INFO:
                __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, fmt, argv);
                break;
            case RETRO_LOG_WARN:
                __android_log_vprint(ANDROID_LOG_WARN, LOG_TAG, fmt, argv);
                break;
            case RETRO_LOG_ERROR:
                __android_log_vprint(ANDROID_LOG_ERROR, LOG_TAG, fmt, argv);
                break;
            default:
                break;
        }
    }

    void Environment::CoreCallbackNotifyAudioState(bool active, unsigned int occupancy, bool underrun_likely) {
        //TODO: 核心通知前端音频状态
    }

    retro_proc_address_t Environment::CoreCallbackGetProcAddress(const char *sym) {
        //LOGD_Env("get proc address: %s", sym);
        return (retro_proc_address_t) eglGetProcAddress(sym);
    }

    const std::string Environment::GetVariable(const std::string &key, const std::string &defaultValue) {
        auto foundVariable = variables.find(key);
        if (foundVariable != variables.end()) {
            return foundVariable->second.value;
        }
        return defaultValue;
    }


}


