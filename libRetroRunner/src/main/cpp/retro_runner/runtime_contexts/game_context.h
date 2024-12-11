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

        inline unsigned int get_geometry_max_height() const { return geometry_max_height_; }

        inline unsigned int get_geometry_max_width() const { return geometry_max_width_; }

        inline unsigned int get_geometry_height() const { return geometry_height_; }

        inline unsigned int get_geometry_width() const { return geometry_width_; }

        inline unsigned int get_geometry_rotation() const { return geometry_rotation_; }

        inline float get_geometry_aspect_ratio() const { return geometry_aspect_ratio_; }

        inline double get_fps() const { return fps_; }

        inline double get_sample_rate() const { return sample_rate_; }

        // Setters
        inline void set_game_path(std::string game_path) { game_path_ = game_path; }

        inline void set_save_path(std::string save_path) { save_path_ = save_path; }

        inline void set_geometry_max_height(unsigned int geometry_max_height) { geometry_max_height_ = geometry_max_height; }

        inline void set_geometry_max_width(unsigned int geometry_max_width) { geometry_max_width_ = geometry_max_width; }

        inline void set_geometry_height(unsigned int geometry_height) { geometry_height_ = geometry_height; }

        inline void set_geometry_width(unsigned int geometry_width) { geometry_width_ = geometry_width; }

        inline void set_geometry_rotation(unsigned int geometry_rotation) { geometry_rotation_ = geometry_rotation; }

        inline void set_geometry_aspect_ratio(float geometry_aspect_ratio) { geometry_aspect_ratio_ = geometry_aspect_ratio; }

        inline void set_fps(double fps) { fps_ = fps; }

        inline void set_sample_rate(double sample_rate) { sample_rate_ = sample_rate; }

    private:
        std::string game_path_;
        std::string save_path_;

        unsigned int geometry_max_height_;
        unsigned int geometry_max_width_;
        unsigned int geometry_height_;
        unsigned int geometry_width_;

        unsigned int geometry_rotation_;

        float geometry_aspect_ratio_;
        double fps_;
        double sample_rate_;
    };

}


#endif
