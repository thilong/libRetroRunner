//
// Created by aidoo on 11/22/2024.
//

#ifndef _ERROR_H
#define _ERROR_H

enum RRError {
    kSuccess = 0,
    kFailed = -1000,
    kAppNotRunning,
    kEmptyMemory,
    kEmptyFile,
    kCannotReadMemory,
    kCannotWriteData,
};


#endif
