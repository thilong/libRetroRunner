//
// Created by aidoo on 2024/11/13.
//

#ifndef _GAME_RUNTIME_CONTEXT_H
#define _GAME_RUNTIME_CONTEXT_H

#include <string>

namespace libRetroRunner {

    class GameRuntimeContext {
    public:
        GameRuntimeContext();

        ~GameRuntimeContext();

    public:

    public:
        // Getters
        inline std::string get_game_path() const { return game_path_; }

        inline std::string get_save_path() const { return save_path_; }

        inline bool get_geometry_changed() const { return geometry_changed_; }

        inline unsigned int get_geometry_max_height() const { return geometry_max_height_; }

        inline unsigned int get_geometry_max_width() const { return geometry_max_width_; }

        inline unsigned int get_geometry_height() const { return geometry_height_; }

        inline unsigned int get_geometry_width() const { return geometry_width_; }

        inline unsigned int get_geometry_rotation() const { return geometry_rotation_; }

        inline float get_geometry_aspect_ratio() const { return geometry_aspect_ratio_; }

        inline float get_fps() const { return fps_; }

        inline float get_forwarding_fps() const { return fps_ * game_speed_; }

        inline float get_sample_rate() const { return sample_rate_; }

        inline float get_game_speed() const { return game_speed_; }

        inline bool get_is_fast_forwarding() const { return game_speed_ > 1.0; }

        // Setters
        inline void set_game_path(std::string game_path) { game_path_ = game_path; }

        inline void set_save_path(std::string save_path) { save_path_ = save_path; }

        inline void set_geometry_changed(bool geometry_changed) { geometry_changed_ = geometry_changed; }

        inline void set_geometry_max_height(unsigned int geometry_max_height) { geometry_max_height_ = geometry_max_height; }

        inline void set_geometry_max_width(unsigned int geometry_max_width) { geometry_max_width_ = geometry_max_width; }

        inline void set_geometry_height(unsigned int geometry_height) { geometry_height_ = geometry_height; }

        inline void set_geometry_width(unsigned int geometry_width) { geometry_width_ = geometry_width; }

        inline void set_geometry_rotation(unsigned int geometry_rotation) { geometry_rotation_ = geometry_rotation; }

        inline void set_geometry_aspect_ratio(float geometry_aspect_ratio) { geometry_aspect_ratio_ = geometry_aspect_ratio; }

        inline void set_fps(float fps) { fps_ = fps; }

        inline void set_sample_rate(float sample_rate) { sample_rate_ = sample_rate; }

        inline void set_game_speed(float game_speed) { game_speed_ = game_speed; }

    private:
        std::string game_path_;
        std::string save_path_;

        bool geometry_changed_ = false;

        unsigned int geometry_max_height_;
        unsigned int geometry_max_width_;
        unsigned int geometry_height_;
        unsigned int geometry_width_;

        unsigned int geometry_rotation_ = 0;

        float geometry_aspect_ratio_;
        float fps_ = 60.0f;
        float sample_rate_;

        float game_speed_ = 1.0;
    };

}


#endif
