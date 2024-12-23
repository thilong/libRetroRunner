//
// Created by Aidoo.TK on 2024/11/6.
//
#include "input_context.h"
#include "software_input.h"
#include "empty_input.h"

#include "../types/log.h"

namespace libRetroRunner {

    InputContext::InputContext() {

    }

    InputContext::~InputContext() {

    }

    std::shared_ptr<InputContext> InputContext::Create(std::string &driver) {
        if (driver == "software") {
            LOGD("[INPUT] Create input context for driver 'software'.");
            return std::make_shared<SoftwareInput>();
        }
        LOGW("[INPUT] Unsupported input driver '%s', empty input context will be used.", driver.c_str());
        return std::make_shared<EmptyInput>();
    }


}
