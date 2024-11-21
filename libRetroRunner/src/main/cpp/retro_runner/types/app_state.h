//
// Created by aidoo on 2024/11/21.
//

#ifndef _APP_STATE_H
#define _APP_STATE_H

enum AppState {
    kNone = 0,
    kPathsReady = 1 << 0,    //1
    kCoreReady = 1 << 1,  //2
    kContentReady = 1 << 2,  //4
    kVideoReady = 1 << 3, //8
    kRunning = 1 << 4,   //16
    kPaused = 1 << 5,
};

#endif
