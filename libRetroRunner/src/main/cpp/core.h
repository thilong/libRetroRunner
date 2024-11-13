//
// Created by aidoo on 2024/10/31.
//

#ifndef LIBRETROSD_CORE_H
#define LIBRETROSD_CORE_H

#include <string>

#include "libretro-common/include/libretro.h"

namespace libRetroRunner {

    class Core {
    public:
        void (*retro_init)(void);

        void (*retro_deinit)(void);

        unsigned (*retro_api_version)(void);

        void (*retro_cheat_reset)(void);

        void (*retro_cheat_set)(unsigned index, bool enabled, const char *code);

        void (*retro_get_system_info)(struct retro_system_info *info);

        void (*retro_get_system_av_info)(struct retro_system_av_info *info);

        void (*retro_set_controller_port_device)(unsigned port, unsigned device);

        void (*retro_reset)(void);

        void (*retro_run)(void);

        size_t (*retro_serialize_size)(void);

        bool (*retro_serialize)(void *data, size_t size);

        bool (*retro_unserialize)(const void *data, size_t size);

        size_t (*retro_get_memory_size)(unsigned id);

        void *(*retro_get_memory_data)(unsigned id);

        bool (*retro_load_game)(const struct retro_game_info *game);

        void (*retro_unload_game)(void);

        void (*retro_set_video_refresh)(retro_video_refresh_t);

        void (*retro_set_environment)(retro_environment_t);

        void (*retro_set_audio_sample)(retro_audio_sample_t);

        void (*retro_set_audio_sample_batch)(retro_audio_sample_batch_t);

        void (*retro_set_input_poll)(retro_input_poll_t);

        void (*retro_set_input_state)(retro_input_state_t);

        Core(const std::string &soCorePath);

        ~Core();

    private:
        void open(const std::string &soCorePath);

        void close();

        void *libHandle = nullptr;
    };

}

#endif //LIBRETROSD_CORE_H
