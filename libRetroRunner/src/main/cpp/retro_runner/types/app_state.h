//
// Created by Aidoo.TK on 2024/11/21.
//

#ifndef _APP_STATE_H
#define _APP_STATE_H

enum AppState {
    kNone = 0,
    kPathsReady = 1 << 0,       // all need paths are set.
    kCoreReady = 1 << 1,        // core has been successfully loaded.
    kContentReady = 1 << 2,     // game content has been loaded.
    kVideoReady = 1 << 3,       // video output is read.
    kRunning = 1 << 4,          // main emu thread is active
    kPaused = 1 << 5,           // main emu step is paused
    kAudioEnabled = 1 << 6      // audio output is enabled.
};

#endif
