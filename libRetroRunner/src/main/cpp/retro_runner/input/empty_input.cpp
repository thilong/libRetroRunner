//
// Created by Aidoo.TK on 11/22/2024.
//
#include "empty_input.h"

namespace libRetroRunner {

    EmptyInput::EmptyInput() {

    }

    EmptyInput::~EmptyInput() {

    }

    void EmptyInput::Init(int max_user) {

    }

    void EmptyInput::UpdateAxis(unsigned int port, unsigned int analog, unsigned int key, float value) {

    }

    bool EmptyInput::UpdateButton(unsigned int port, unsigned int button, bool pressed) {
        return false;
    }

    int16_t EmptyInput::State(unsigned int port, unsigned int device, unsigned int index, unsigned int id) {
        return 0;
    }

    void EmptyInput::Poll() {

    }

    void EmptyInput::Destroy() {

    }
}