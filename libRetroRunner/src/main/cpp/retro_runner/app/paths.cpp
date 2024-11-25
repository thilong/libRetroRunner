//
// Created by aidoo on 2024/11/21.
//
#include "paths.h"

void libRetroRunner::Paths::SetPaths(const std::string &rom, const std::string &core, const std::string &system, const std::string &save) {
    rom_path_ = rom;
    core_path_ = core;
    system_path_ = system;
    save_path_ = save;

    char pathSeparator = '/';
    //get rom name and extension
    size_t posLastFolderSep = rom.find_last_of(pathSeparator);
    if (posLastFolderSep != std::string::npos) {
        rom_name_ = rom.substr(posLastFolderSep + 1);
        size_t pos = rom_name_.find_last_of('.');
        if (pos != std::string::npos) {
            rom_ext_ = rom_name_.substr(pos);
            rom_name_ = rom_name_.substr(0, pos);
        } else {
            rom_ext_ = "";
        }
    } else {
        rom_name_ = rom;
        size_t pos = rom_name_.find_last_of('.');
        if (pos != std::string::npos) {
            rom_ext_ = rom_name_.substr(pos);
            rom_name_ = rom_name_.substr(0, pos);
        } else {
            rom_ext_ = "";
        }
    }
    //get rom folder
    if (posLastFolderSep != std::string::npos) {
        rom_folder_ = rom.substr(0, posLastFolderSep);
    } else {
        rom_folder_ = "";
    }


}

std::string libRetroRunner::Paths::GetSaveRamPath() {
    return rom_path_ + ".srm";
}

std::string libRetroRunner::Paths::GetSaveStatePath(int idx) {
    return rom_path_ + ".state" + std::to_string(idx);
}

std::string libRetroRunner::Paths::GetScreenshotPathForSaveState(int idx) {
    return rom_path_ + ".state" + std::to_string(idx) + ".png";
}

