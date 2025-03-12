//
// Created by Aidoo.TK on 2024/11/13.
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


        inline bool UseLowLatency() {
            return low_latency_;
        }

        inline int GetMaxPlayerCount() {
            return max_player_count_;
        }

        inline std::string &GetVideoDriver() {
            return video_driver_;
        }

        inline std::string &GetInputDriver() {
            return input_driver_;
        }

        inline std::string &GetAudioDriver() {
            return audio_driver_;
        }

        /**
         * if use linear in video output.
         * @return linear
         */
        inline bool GetVideoUseLinear() {
            return video_linear;
        }

    private:
        std::string video_driver_;
        std::string input_driver_;
        std::string audio_driver_;

        bool low_latency_ = true;
        int max_player_count_ = 4;
        bool video_linear = false;
    };

}


#endif
