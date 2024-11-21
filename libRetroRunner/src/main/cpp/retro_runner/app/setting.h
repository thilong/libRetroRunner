//
// Created by aidoo on 2024/11/13.
//

#ifndef _SETTING_H
#define _SETTING_H

#include <string>

namespace libRetroRunner {
    /**
     * Global setting for app
     */
    class Setting {
    public:
        Setting();

        ~Setting();

        static Setting *Current();


        inline bool IsLowLatency() {
            return low_latency_;
        }

        inline int MaxPlayerCount() {
            return max_player_count_;
        }

    public:

        bool low_latency_ = true;
        int max_player_count_ = 4;
    };

}


#endif
