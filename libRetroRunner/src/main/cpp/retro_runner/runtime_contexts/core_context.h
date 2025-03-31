//
// Created by Aidoo.TK on 2024/11/13.
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
        friend class Environment;

    public:
        CoreRuntimeContext();

        ~CoreRuntimeContext();

    public:
        //getter
        inline std::string GetCorePath() const { return core_path_; }

        inline std::string GetSystemPath() const { return system_path_; }

        inline const std::map<unsigned int, std::string> &GetSupportControllers() { return support_controllers_; }

        inline unsigned int GetLanguage() const { return language_; }

        inline unsigned int GetMaxUserCount() const { return max_user_count_; }

        inline bool GetSupportNoGame() const { return support_no_game_; }

        inline int GetPixelFormat() const { return pixel_format_; }

        inline int GetRenderContextType() const { return render_context_type_; }

        inline int GetRenderMajorVersion() const { return render_major_version_; }

        inline int GetRenderMinorVersion() const { return render_minor_version_; }

        inline bool GetRenderUseHardwareAcceleration() const { return render_hardware_acceleration_; }

        inline bool GetRenderUseDepth() const { return render_depth_; }

        inline bool GetRenderUseStencil() const { return render_stencil_; }

        inline retro_hw_context_reset_t GetRenderHWContextResetCallback() const { return render_hw_context_reset_; }

        inline retro_hw_context_reset_t GetRenderHWContextDestroyCallback() const { return render_hw_context_destroy_; }

        inline struct retro_hw_render_context_negotiation_interface *GetRenderHWNegotiationInterface() const { return negotiation_interface_; }

        //setter
        inline void SetCorePath(std::string core_path) { core_path_ = core_path; }

        inline void SetSystemPath(std::string system_path) { system_path_ = system_path; }

        inline void SetSupportController(int key, std::string value) { support_controllers_[key] = value; }


        inline void SetMaxUserCount(unsigned int max_user_count) { max_user_count_ = max_user_count; }

        inline void SetSupportNoGame(bool support_no_game) { support_no_game_ = support_no_game; }

        inline void SetPixelFormat(int pixel_format) { pixel_format_ = pixel_format; }

        inline void SetRenderContextType(int render_context_type) { render_context_type_ = render_context_type; }

        inline void SetRenderMajorVersion(int render_major_version) { render_major_version_ = render_major_version; }

        inline void SetRenderMinorVersion(int render_minor_version) { render_minor_version_ = render_minor_version; }

        inline void SetRenderUseHardwareAcceleration(bool render_hardware_acceleration) { render_hardware_acceleration_ = render_hardware_acceleration; }

        inline void SetRenderUseDepth(bool render_depth) { render_depth_ = render_depth; }

        inline void SetRenderUseStencil(bool render_stencil) { render_stencil_ = render_stencil; }

        inline void SetRenderHWContextResetCallback(retro_hw_context_reset_t render_hw_context_reset) { render_hw_context_reset_ = render_hw_context_reset; }

        inline void SetRenderHWContextDestroyCallback(retro_hw_context_reset_t render_hw_context_destroy) { render_hw_context_destroy_ = render_hw_context_destroy; }

        inline void SetRenderHWNegotiationInterface(const struct retro_hw_render_context_negotiation_interface *negotiation_interface) { negotiation_interface_ = negotiation_interface; }

    private:

        std::string core_path_;
        std::string system_path_;

        std::map<unsigned int, std::string> support_controllers_;

        unsigned int language_ = RETRO_LANGUAGE_ENGLISH;
        unsigned int max_user_count_ = 4;


        bool support_no_game_;


        int pixel_format_;

        int render_context_type_;
        int render_major_version_;
        int render_minor_version_;


        bool render_hardware_acceleration_;
        bool render_depth_;
        bool render_stencil_;

        retro_hw_context_reset_t render_hw_context_reset_;
        retro_hw_context_reset_t render_hw_context_destroy_;


        const struct retro_hw_render_context_negotiation_interface *negotiation_interface_;


        int serialization_quirks_;
    };

}


#endif
