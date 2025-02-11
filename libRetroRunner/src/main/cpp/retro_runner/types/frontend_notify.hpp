//
// Created by Aidoo.TK on 2024/12/17.
//

#ifndef LIBRETRORUNNER_FRONTEND_NOTIFY_HPP
#define LIBRETRORUNNER_FRONTEND_NOTIFY_HPP

#include <memory>
#include "object_ref.hpp"
#include "log.h"

namespace libRetroRunner {

    enum AppNotifications {
        kAppNotificationContentLoaded = 100,
        kAppNotificationTerminated,
        kAppNotificationGameGeometryChanged
    };

    class FrontendNotifyObject : public RRObjectRef {
    public:
        explicit FrontendNotifyObject(int type) { notify_type_ = type; }

        ~FrontendNotifyObject() override = default;

        inline int GetNotifyType() const {
            return notify_type_;
        }

    protected:
        int notify_type_;
    };


    template<typename T>
    class FrontendNotify : public FrontendNotifyObject {
    public:
        explicit FrontendNotify(int type) : FrontendNotifyObject(type) {
            data_ = nullptr;
        }

        FrontendNotify(int type, std::shared_ptr<T> &data) : FrontendNotifyObject(type), data_(data) {

        }

        ~FrontendNotify() override {
            data_ = nullptr;
        }

        inline std::shared_ptr<T> GetData() {
            return data_;
        }

    private:
        std::shared_ptr<T> data_;
    };

    /*
     * callback function type for frontend notify
     * data should be type of FrontendNotify, and use Retain, Release to manage memory.
     */
    typedef void (*FrontendNotifyCallback)(void *data);

}
#endif //LIBRETRORUNNER_FRONTEND_NOTIFY_HPP
