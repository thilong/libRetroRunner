//
// Created by aidoo on 2024/11/6.
//
#include "input_context.h"
#include "input/software_input.h"

namespace libRetroRunner {

    InputContext::InputContext() {

    }

    InputContext::~InputContext() {

    }

    std::unique_ptr<InputContext> InputContext::NewInstance() {
        return std::make_unique<SoftwareInput>();
    }

}
