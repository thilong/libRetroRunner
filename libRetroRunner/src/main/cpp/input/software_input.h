//
// Created by aidoo on 2024/11/6.
//

#ifndef _SOFTWARE_INPUT_H
#define _SOFTWARE_INPUT_H

#include "../input_context.h"

namespace libRetroRunner {
    class SoftwareInput : public InputContext {
    public:
        SoftwareInput();

        ~SoftwareInput() override;

        void Init() override;

        void UpdateAxis(unsigned int port, unsigned int id, float value) override;

        void UpdateButton(unsigned int port, unsigned int button, bool pressed) override;


        int16_t State(unsigned int port, unsigned int device, unsigned int index, unsigned int id) override;

        void Poll() override;

        void Destroy() override;

    private:
        bool have_button_map[4];

        int16_t button_map[MAX_PLAYER][256];
        int16_t buttons[MAX_PLAYER][256];  //一个玩家最多256个按钮
        int16_t axis[MAX_PLAYER][8];    //一个玩家最多4个摇杆，每个摇杆有两个轴(x, y)
    };
}
#endif
