//
// Created by Aidoo.TK on 2024/11/13.
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
        inline std::string GetGamePath() const { return game_path_; }

        inline std::string GetSavePath() const { return save_path_; }

        inline bool GetIsGeometryChanged() const { return geometry_changed_; }

        inline unsigned int GetGeometryMaxHeight() const { return geometry_max_height_; }

        inline unsigned int GetGeometryMaxWidth() const { return geometry_max_width_; }

        inline unsigned int GetGeometryHeight() const { return geometry_height_; }

        inline unsigned int GetGeometryWidth() const { return geometry_width_; }

        inline unsigned int GetGeometryRotation() const { return geometry_rotation_; }

        inline float GetGeometryAspectRatio() const { return geometry_aspect_ratio_; }

        inline float GetFps() const { return fps_; }

        inline float GetFastForwardingFps() const { return fps_ * game_speed_; }

        inline float GetSampleRate() const { return sample_rate_; }

        inline float GetGameSpeed() const { return game_speed_; }

        inline bool GetIsFastForwarding() const { return game_speed_ > 1.0; }

        std::string GetSaveStateFilePath(int slot);

        // Setters
        inline void SetGamePath(std::string game_path) { game_path_ = game_path; }

        inline void SetSavePath(std::string save_path) { save_path_ = save_path; }

        inline void SetGeometryChanged(bool geometry_changed) { geometry_changed_ = geometry_changed; }

        inline void SetGeometryMaxHeight(unsigned int geometry_max_height) { geometry_max_height_ = geometry_max_height; }

        inline void SetGeometryMaxWidth(unsigned int geometry_max_width) { geometry_max_width_ = geometry_max_width; }

        inline void SetGeometryHeight(unsigned int geometry_height) { geometry_height_ = geometry_height; }

        inline void SetGeometryWidth(unsigned int geometry_width) { geometry_width_ = geometry_width; }

        inline void SetGeometryRotation(unsigned int geometry_rotation) { geometry_rotation_ = geometry_rotation; }

        inline void SetGeometryAspectRatio(float geometry_aspect_ratio) { geometry_aspect_ratio_ = geometry_aspect_ratio; }

        inline void SetFps(float fps) { fps_ = fps; }

        inline void SetSampleRate(float sample_rate) { sample_rate_ = sample_rate; }

        inline void SetGameSpeed(float game_speed) { game_speed_ = game_speed; }

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
