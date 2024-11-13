//
// Created by aidoo on 2024/11/6.
//

#ifndef _INPUT_CONTEXT_H
#define _INPUT_CONTEXT_H

#include <stdint.h>
#include <memory>

#define MAX_PLAYER 4

namespace libRetroRunner {
    class InputContext {
    public:
        InputContext();

        virtual ~InputContext();

        virtual void Init() = 0;

        virtual void UpdateAxis(unsigned int port, unsigned int id, float value) = 0;

        virtual void UpdateButton(unsigned int port, unsigned int button, bool pressed) = 0;

        virtual int16_t State(unsigned int port, unsigned int device, unsigned int index, unsigned int id) = 0;

        virtual void Poll() = 0;

        virtual void Destroy() = 0;

    public:
        static std::unique_ptr<InputContext> NewInstance();
    };
}
#endif
