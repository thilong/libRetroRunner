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

        inline int GetMaxPlayerCount() {
            return max_player_count_;
        }

        inline std::string& GetVideoDriver() {
            return video_driver_;
        }

        /**
         * if use linear in video output.
         * @return linear
         */
        inline bool GetVideoUseLinear(){
            return video_linear;
        }
    private:
        std::string video_driver_;
        bool low_latency_ = true;
        int max_player_count_ = 4;
        bool video_linear = true;
    };

}


#endif
