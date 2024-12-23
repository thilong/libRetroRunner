//
// Created by Aidoo.TK on 11/22/2024.
//

#ifndef _EMPTY_INPUT_H
#define _EMPTY_INPUT_H

#include "input_context.h"

namespace libRetroRunner {
    class EmptyInput : public InputContext {
    public:
        EmptyInput();

        ~EmptyInput() override;

        void Init(int max_user) override;

        void UpdateAxis(unsigned int port, unsigned int analog, unsigned int key, float value) override;

        bool UpdateButton(unsigned int port, unsigned int button, bool pressed) override;


        int16_t State(unsigned int port, unsigned int device, unsigned int index, unsigned int id) override;

        void Poll() override;

        void Destroy() override;

    };
}
#endif
