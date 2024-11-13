//
// Created by aidoo on 2024/11/1.
//

#ifndef _RR_TYPES_H
#define _RR_TYPES_H

#define BIT_SET(_VAR_, _VALUE_) ((_VAR_) |= (_VALUE_))
#define BIT_DELETE(_VAR_, _VALUE_) ((_VAR_) &= (~(_VALUE_)))
#define BIT_TEST(_VAR_, _VALUE_) (((_VAR_) & (_VALUE_)) != 0)

#include <string>

struct Variable {
    std::string key;
    std::string value;
    std::string description;
};

enum AppCommands {
    kNone = 10,
    kLoadCore,
    kLoadContent,
    kInitVideo,
    kInitInput,
    kInitAudio,
    kUnloadVideo,
    kResetGame,
    kPauseGame,
    kStopGame,

    kEnableAudio,
    kDisableAudio
};



#endif
