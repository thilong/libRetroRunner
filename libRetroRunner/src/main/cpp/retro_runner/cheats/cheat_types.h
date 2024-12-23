//
// Created by Aidoo.TK on 2024/11/25.
//

#ifndef _CHEAT_TYPES_H
#define _CHEAT_TYPES_H

#include <string>

namespace libRetroRunner {
    class Cheat {
        friend class CheatManager;

    public:
        long id;
        bool enabled;
        std::string code;
        std::string description;

        int index;
    };
}

#endif
