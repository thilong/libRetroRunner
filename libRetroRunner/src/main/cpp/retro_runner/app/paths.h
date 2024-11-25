//
// Created by aidoo on 2024/11/21.
//

#ifndef _PATHS_H
#define _PATHS_H

#include <string>

namespace libRetroRunner {
    class Paths {
    public:
        Paths() {};

        ~Paths() {};

        void SetPaths(const std::string &rom, const std::string &core, const std::string &system, const std::string &save);

        inline const std::string &GetRomName() const {
            return rom_name_;
        }

        inline const std::string &GetRomExt() const {
            return rom_ext_;
        }

        inline const std::string &GetRomFolder() const {
            return rom_folder_;
        }

        inline const std::string &GetRomPath() const {
            return rom_path_;
        }

        inline const std::string &GetCorePath() const {
            return core_path_;
        }

        inline const std::string &GetSystemPath() const {
            return system_path_;
        }

        inline const std::string &GetSavePath() const {
            return save_path_;
        }

        std::string GetSaveRamPath();
        std::string GetSaveStatePath(int idx);
        std::string GetScreenshotPathForSaveState(int idx);

    private:
        /* rom name, without extension */
        std::string rom_name_;
        /* rom file extension, with '.' */
        std::string rom_ext_;
        /* path where rom file stored  */
        std::string rom_folder_;

        std::string rom_path_;
        /* path where core file stored, when it's in lib, it's only a file name. */
        std::string core_path_;
        /* path where system files stored, like bios, etc. */
        std::string system_path_;
        /* path where save files stored  */
        std::string save_path_;
    };
}
#endif
