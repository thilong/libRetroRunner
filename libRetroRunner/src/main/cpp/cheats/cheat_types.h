//
// Created by aidoo on 2024/11/18.
//

#ifndef _CHEAT_TYPES_H
#define _CHEAT_TYPES_H

#include <string>
#include <memory>
#include <map>
namespace libRetroRunner {
    struct Cheat {
        /**
         * 给前端使用的id，动态生成
         */
        long id;
        /**
         * 用于核心内部使用的ID，不能在前端使用
         */
        int index;              //用于核心内部索引
        bool enabled;
        std::string code;
        std::string description;
    };
}
#endif
