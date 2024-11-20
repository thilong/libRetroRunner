//
// Created by aidoo on 2024/11/6.
//
#include "software_input.h"
#include <android/input.h>
#include "../libretro-common/include/libretro.h"
#include "../rr_log.h"
#include "../environment.h"
#include "../app.h"

namespace libRetroRunner {

    int supportKeys[256] = {0};

    void _initDefaultButtonMap(int16_t *map_arrays, bool *map_state) {
        for (int port = 0; port < MAX_PLAYER; port++) {
            int16_t *map = map_arrays + port * 256;

            map[RETRO_DEVICE_ID_JOYPAD_A] = RETRO_DEVICE_ID_JOYPAD_A;
            map[RETRO_DEVICE_ID_JOYPAD_B] = RETRO_DEVICE_ID_JOYPAD_B;
            map[RETRO_DEVICE_ID_JOYPAD_X] = RETRO_DEVICE_ID_JOYPAD_X;
            map[RETRO_DEVICE_ID_JOYPAD_Y] = RETRO_DEVICE_ID_JOYPAD_Y;
            map[RETRO_DEVICE_ID_JOYPAD_L] = RETRO_DEVICE_ID_JOYPAD_L;
            map[RETRO_DEVICE_ID_JOYPAD_R] = RETRO_DEVICE_ID_JOYPAD_R;
            map[RETRO_DEVICE_ID_JOYPAD_SELECT] = RETRO_DEVICE_ID_JOYPAD_SELECT;
            map[RETRO_DEVICE_ID_JOYPAD_START] = RETRO_DEVICE_ID_JOYPAD_START;
            map[RETRO_DEVICE_ID_JOYPAD_L3] = RETRO_DEVICE_ID_JOYPAD_L3;
            map[RETRO_DEVICE_ID_JOYPAD_R3] = RETRO_DEVICE_ID_JOYPAD_R3;
            map[RETRO_DEVICE_ID_JOYPAD_UP] = RETRO_DEVICE_ID_JOYPAD_UP;
            map[RETRO_DEVICE_ID_JOYPAD_DOWN] = RETRO_DEVICE_ID_JOYPAD_DOWN;
            map[RETRO_DEVICE_ID_JOYPAD_LEFT] = RETRO_DEVICE_ID_JOYPAD_LEFT;
            map[RETRO_DEVICE_ID_JOYPAD_RIGHT] = RETRO_DEVICE_ID_JOYPAD_RIGHT;
            map[RETRO_DEVICE_ID_JOYPAD_L2] = RETRO_DEVICE_ID_JOYPAD_L2;
            map[RETRO_DEVICE_ID_JOYPAD_R2] = RETRO_DEVICE_ID_JOYPAD_R2;

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

            map_state[port] = true;
        }
        for (int retroId = 0; retroId < 16; retroId++) {
            supportKeys[retroId] = 1;
        }
        supportKeys[AKEYCODE_BUTTON_A] = 1;
        supportKeys[AKEYCODE_BUTTON_B] = 1;
        supportKeys[AKEYCODE_BUTTON_X] = 1;
        supportKeys[AKEYCODE_BUTTON_Y] = 1;
        supportKeys[AKEYCODE_BUTTON_L1] = 1;
        supportKeys[AKEYCODE_BUTTON_R1] = 1;
        supportKeys[AKEYCODE_BUTTON_SELECT] = 1;
        supportKeys[AKEYCODE_BUTTON_START] = 1;
        supportKeys[AKEYCODE_BUTTON_THUMBL] = 1;
        supportKeys[AKEYCODE_BUTTON_THUMBR] = 1;
        supportKeys[AKEYCODE_DPAD_UP] = 1;
        supportKeys[AKEYCODE_DPAD_DOWN] = 1;
        supportKeys[AKEYCODE_DPAD_LEFT] = 1;
        supportKeys[AKEYCODE_DPAD_RIGHT] = 1;
        supportKeys[AKEYCODE_BUTTON_L2] = 1;
        supportKeys[AKEYCODE_BUTTON_R2] = 1;


    }

    SoftwareInput::SoftwareInput() {

    }

    SoftwareInput::~SoftwareInput() {

    }

    void SoftwareInput::Init() {
        memset(button_map, -1, sizeof(button_map));
        memset(buttons, 0, sizeof(buttons));
        memset(axis, 0, sizeof(axis));
        memset(have_button_map, 0, sizeof(have_button_map));

        _initDefaultButtonMap((int16_t *) button_map, (bool *) have_button_map);

        AppContext *app = AppContext::Current();
        Environment *environment = app->GetEnvironment();
        int device = RETRO_DEVICE_JOYPAD;
        if (environment->supportControllers.find(RETRO_DEVICE_ANALOG) != environment->supportControllers.end()) {
            device = RETRO_DEVICE_ANALOG;
        }
        for (int i = 0; i < MAX_PLAYER; ++i) {
            app->SetController(i, device);
        }
    }

    int16_t SoftwareInput::State(unsigned int port, unsigned int device, unsigned int index, unsigned int id) {
        /**
         * port: 玩家index
         * device: 设备类型
         * index: 设备索引, 比如多个摇杆0-3
         * id: 按键索引, eg:RETRO_DEVICE_ID_JOYPAD_A
         */
        switch (device) {
            case RETRO_DEVICE_JOYPAD:
                if (id == RETRO_DEVICE_ID_JOYPAD_MASK) {
                    //使用mask, 一次性返回所有的按钮状态 0 - 16
                    unsigned i;
                    int16_t ret = 0;
                    for (i = 0; i < 16; i++) {
                        if (buttons[port][i] > 0) {
                            ret |= (1 << i);
                        }
                    }
                    return ret;
                }

                return buttons[port][id];
                break;
            case RETRO_DEVICE_ANALOG: {
                unsigned id_minus = 0;
                unsigned id_plus = 0;
                unsigned buttonMinusIdx = (index * 4) + id * 2;
                unsigned buttonPlusIdx = (index * 4) + id * 2 + 1;
                int16_t ret = 0;
                if (axis[port][buttonPlusIdx] != 0) {
                    ret = 0x7fff;
                }
                if (axis[port][buttonMinusIdx] != 0) {
                    ret += (-0x7fff);
                }
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
        memset(button_map, 0, sizeof(button_map));
        memset(buttons, 0, sizeof(buttons));
        memset(axis, 0, sizeof(axis));
    }

    void SoftwareInput::UpdateAxis(unsigned int port, unsigned int id, float value) {

    }

    bool SoftwareInput::UpdateButton(unsigned int port, unsigned int button, bool pressed) {
        if (supportKeys[button] != 1) return false;
        int mappedButton = button;
        if (have_button_map[port]) {
            //如果这个玩家映射了按钮
            mappedButton = button_map[port][button];

        }
        if (mappedButton >= 0) {
            buttons[port][mappedButton] = pressed ? 1 : 0;
        }
        buttons[port][button] = pressed ? 1 : 0;
        return true;
    }

    void SoftwareInput::Poll() {

    }
}