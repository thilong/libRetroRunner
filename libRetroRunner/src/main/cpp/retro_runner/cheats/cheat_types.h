//
// Created by Aidoo.TK on 2024/11/25.
//

#ifndef _CHEAT_TYPES_H
#define _CHEAT_TYPES_H

#include <string>

namespace libRetroRunner {
    struct Cheat {
        /* id of cheat */
        long id;
        bool enabled;
        std::string code;
        std::string description;

        /* cheat index in core.*/
        int index;
    };
}

#endif
