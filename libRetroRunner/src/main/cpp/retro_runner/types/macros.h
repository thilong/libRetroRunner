//
// Created by aidoo on 2024/11/21.
//

#ifndef _MACROS_H
#define _MACROS_H


#define BIT_SET(_VAR_, _VALUE_) ((_VAR_) |= (_VALUE_))
#define BIT_UNSET(_VAR_, _VALUE_) ((_VAR_) &= (~(_VALUE_)))
#define BIT_TEST(_VAR_, _VALUE_) (((_VAR_) & (_VALUE_)) != 0)


#endif
