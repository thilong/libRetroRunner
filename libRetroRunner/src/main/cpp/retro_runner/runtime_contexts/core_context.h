//
// Created by aidoo on 2024/11/13.
//

#ifndef _CORE_RUNTIME_CONTEXT_H
#define _CORE_RUNTIME_CONTEXT_H

#include <string>
#include <map>
#include <unordered_map>
#include <libretro-common/include/libretro.h>

#include <retro_runner/types/variable.h>

namespace libRetroRunner {

    class CoreRuntimeContext {
    public:
        CoreRuntimeContext();

        ~CoreRuntimeContext();

    public:
        inline void set_variables_changed(bool flag) { variables_changed_ = flag; }

        inline bool get_variables_changed() const { return variables_changed_; }

    public:
        //getter
        inline bool get_audio_enabled() const { return audio_enabled_; }

        inline bool get_video_enabled() const { return video_enabled_; }

        inline unsigned int get_max_user_count() const { return max_user_count_; }

        inline bool get_support_no_game() const { return support_no_game_; }

        inline int get_render_context_type() const { return render_context_type_; }

        inline int get_render_major_version() const { return render_major_version_; }

        inline int get_render_minor_version() const { return render_minor_version_; }

        inline bool get_render_hardware_acceleration() const { return render_hardware_acceleration_; }

        inline bool get_render_depth() const { return render_depth_; }

        inline bool get_render_stencil() const { return render_stencil_; }

        inline retro_hw_context_reset_t get_render_hw_context_reset() const { return render_hw_context_reset_; }

        inline retro_hw_context_reset_t get_render_hw_context_destroy() const { return render_hw_context_destroy_; }

        //setter
        inline void set_audio_enabled(bool audio_enabled) { audio_enabled_ = audio_enabled; }

        inline void set_video_enabled(bool video_enabled) { video_enabled_ = video_enabled; }

        inline void set_max_user_count(unsigned int max_user_count) { max_user_count_ = max_user_count; }

        inline void set_support_no_game(bool support_no_game) { support_no_game_ = support_no_game; }

        inline void set_render_context_type(int render_context_type) { render_context_type_ = render_context_type; }

        inline void set_render_major_version(int render_major_version) { render_major_version_ = render_major_version; }

        inline void set_render_minor_version(int render_minor_version) { render_minor_version_ = render_minor_version; }

        inline void set_render_hardware_acceleration(bool render_hardware_acceleration) { render_hardware_acceleration_ = render_hardware_acceleration; }

        inline void set_render_depth(bool render_depth) { render_depth_ = render_depth; }

        inline void set_render_stencil(bool render_stencil) { render_stencil_ = render_stencil; }

        inline void set_render_hw_context_reset(retro_hw_context_reset_t render_hw_context_reset) { render_hw_context_reset_ = render_hw_context_reset; }

        inline void set_render_hw_context_destroy(retro_hw_context_reset_t render_hw_context_destroy) { render_hw_context_destroy_ = render_hw_context_destroy; }

    private:

        std::unordered_map<std::string, Variable> variables_;
        bool variables_changed_;
        std::map<int, std::string> support_controllers_;

        bool audio_enabled_;
        bool video_enabled_;
        unsigned int max_user_count_;


        bool support_no_game_;

        int render_context_type_;
        int render_major_version_;
        int render_minor_version_;


        bool render_hardware_acceleration_;
        bool render_depth_;
        bool render_stencil_;

        retro_hw_context_reset_t render_hw_context_reset_;
        retro_hw_context_reset_t render_hw_context_destroy_;

    };

}


#endif
