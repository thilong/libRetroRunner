//
// Created by Aidoo.TK on 2024/11/6.
//
#include "software_input.h"
#include <android/input.h>
#include <libretro-common/include/libretro.h>
#include <retro_runner/types/log.h>

#include <retro_runner/app/environment.h>
#include <retro_runner/app/app_context.h>

#define LOGD_SInput(...) LOGD("[INPUT] "  __VA_ARGS__)

namespace libRetroRunner {

    SoftwareInput::SoftwareInput() {
        max_user_ = 0;
        std::memset(keyboard_state, false, 256);
    }

    SoftwareInput::~SoftwareInput() {

    }

    void SoftwareInput::initDefaultButtonMap() {
        for (int idx = 0; idx < max_user_; idx++) {
            std::map<unsigned, unsigned> &map = button_map[idx];
            for (int retro_key_id = 0; retro_key_id < 16; retro_key_id++) {
                map[retro_key_id] = retro_key_id;
            }

#ifdef ANDROID
            map[AKEYCODE_BUTTON_A] = RETRO_DEVICE_ID_JOYPAD_A;
            map[AKEYCODE_BUTTON_B] = RETRO_DEVICE_ID_JOYPAD_B;
            map[AKEYCODE_BUTTON_X] = RETRO_DEVICE_ID_JOYPAD_X;
            map[AKEYCODE_BUTTON_Y] = RETRO_DEVICE_ID_JOYPAD_Y;
            map[AKEYCODE_BUTTON_L1] = RETRO_DEVICE_ID_JOYPAD_L;
            map[AKEYCODE_BUTTON_R1] = RETRO_DEVICE_ID_JOYPAD_R;
            map[AKEYCODE_BUTTON_SELECT] = RETRO_DEVICE_ID_JOYPAD_SELECT;
            map[AKEYCODE_BUTTON_START] = RETRO_DEVICE_ID_JOYPAD_START;
            map[AKEYCODE_BUTTON_THUMBL] = RETRO_DEVICE_ID_JOYPAD_L3;
            map[AKEYCODE_BUTTON_THUMBR] = RETRO_DEVICE_ID_JOYPAD_R3;
            map[AKEYCODE_DPAD_UP] = RETRO_DEVICE_ID_JOYPAD_UP;
            map[AKEYCODE_DPAD_DOWN] = RETRO_DEVICE_ID_JOYPAD_DOWN;
            map[AKEYCODE_DPAD_LEFT] = RETRO_DEVICE_ID_JOYPAD_LEFT;
            map[AKEYCODE_DPAD_RIGHT] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
            map[AKEYCODE_BUTTON_L2] = RETRO_DEVICE_ID_JOYPAD_L2;
            map[AKEYCODE_BUTTON_R2] = RETRO_DEVICE_ID_JOYPAD_R2;
#endif

        }
    }

    void SoftwareInput::Init(int max_user) {
        max_user_ = max_user;
        for (int idx = 0; idx < max_user_; idx++) {
            button_state[idx] = std::vector<bool>(256);
            axis_state[idx] = std::vector<float>(8);
        }

        initDefaultButtonMap();

        auto app = AppContext::Current();
        auto coreCtx = app->GetCoreRuntimeContext();
        int device = RETRO_DEVICE_JOYPAD;
        auto supportControllers = coreCtx->GetSupportControllers();

        if (supportControllers.find(RETRO_DEVICE_ANALOG) != supportControllers.end()) {
            device = RETRO_DEVICE_ANALOG;
        }
        for (int i = 0; i < max_user; ++i) {
            app->SetController(i, device);
            LOGD_SInput("SetController: [%d -> %d]", i, device);
        }
        LOGD_SInput("SoftwareInput initialized");
    }

    int16_t SoftwareInput::State(unsigned int port, unsigned int device, unsigned int index, unsigned int id) {
        /**
         * port: player port
         * device: 设备类型
         * index: 设备索引, 比如多个摇杆0-3
         * id: 按键索引, eg:RETRO_DEVICE_ID_JOYPAD_A
         * @see libretro.h:381
         */
        switch (device) {
            case RETRO_DEVICE_JOYPAD: {
                auto states = button_state[port];
                if (id == RETRO_DEVICE_ID_JOYPAD_MASK) {
                    int16_t ret = 0;
                    for (unsigned i = 0; i < 16; i++) {
                        if (states[i]) {
                            ret |= (1 << i);
                        }
                    }
                    return ret;
                }
                return states[id] ? 1 : 0;
            }
            case RETRO_DEVICE_ANALOG: {
                int16_t ret = 0;
                if (index > 3 || id > 1) return ret;
                /* index: 0-3 analog
                 * id: analog key id: RETRO_DEVICE_ID_ANALOG_X 0, RETRO_DEVICE_ID_ANALOG_Y 1
                 */
                auto axis = axis_state[port];
                int retro_device_id = (index * 2) + id;
                float value = axis[retro_device_id];
                ret = 0x7fff * value;
                return ret;
            }
            case RETRO_DEVICE_MOUSE:
                break;
            case RETRO_DEVICE_POINTER:
                break;
            case RETRO_DEVICE_KEYBOARD:
                break;
            case RETRO_DEVICE_LIGHTGUN:
                break;
        }

        return 0;
    }

    void SoftwareInput::Destroy() {
        std::memset(keyboard_state, false, 256);
    }

    void SoftwareInput::UpdateAxis(unsigned int port, unsigned int analog, unsigned int key, float value) {
        if (port >= max_user_) return;
        if (analog > 3 || key > 1) return;
        if (value > 1.0 || value < -1.0) return;
        int retro_device_id = (analog * 2) + key;
        axis_state[port][retro_device_id] = value;
    }

    bool SoftwareInput::UpdateButton(unsigned int port, unsigned int button, bool pressed) {
        if (port >= max_user_) return false;
        auto map = button_map[port];

        int mappedButton = button;
        auto map_record = map.find(button);
        if (map_record != map.end()) {
            mappedButton = map_record->second;
        }
        button_state[port][mappedButton] = pressed;
        return true;
    }

    void SoftwareInput::Poll() {

    }


}