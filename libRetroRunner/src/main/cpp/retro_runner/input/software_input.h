//
// Created by aidoo on 2024/11/6.
//

#ifndef _SOFTWARE_INPUT_H
#define _SOFTWARE_INPUT_H

#include <map>
#include "input_context.h"

namespace libRetroRunner {
    class SoftwareInput : public InputContext {
    public:
        SoftwareInput();

        ~SoftwareInput() override;

        void Init(int max_user) override;

        void UpdateAxis(unsigned int port, unsigned int analog, unsigned int key, float value) override;

        bool UpdateButton(unsigned int port, unsigned int button, bool pressed) override;

        int16_t State(unsigned int port, unsigned int device, unsigned int index, unsigned int id) override;

        void Poll() override;

        void Destroy() override;

    private:
        void initDefaultButtonMap();

    private:
        unsigned max_user_;
        bool keyboard_state[256]; //state for keyboard keys
        std::map<unsigned, std::map<unsigned, unsigned> > button_map;    //button map for each player
        std::map<unsigned, std::vector<bool> > button_state;             //button state for each player
        std::map<unsigned, std::vector<float> > axis_state;              //axis state for each player

    };
}
#endif
