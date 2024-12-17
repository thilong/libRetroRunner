//
// Created by aidoo on 2024/12/17.
//

#ifndef LIBRETRORUNNER_OBJECT_REF_HPP
#define LIBRETRORUNNER_OBJECT_REF_HPP

#include "log.h"

namespace libRetroRunner {
    class RRObjectRef {
    public:
        RRObjectRef() : ref_count_(1) {}

        virtual ~RRObjectRef() = default;

        void Retain() {
            ref_count_++;
        }

        void Release() {
            ref_count_--;
            if (ref_count_ < 1) {
                delete this;
            }
        }

    private:
        int ref_count_;
    };
}
#endif //LIBRETRORUNNER_OBJECT_REF_HPP
