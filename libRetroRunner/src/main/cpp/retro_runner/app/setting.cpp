//
// Created by aidoo on 2024/11/13.
//

#include "setting.h"
#include "utils/utils.h"

namespace libRetroRunner {
    Setting::Setting() {

    }

    Setting::~Setting() {

    }

    void Setting::InitWithPaths(const std::string &gamePath, const std::string &savePath) {
        saveRamPath = Utils::getFilePathWithoutExtension(gamePath) + ".srm";
        saveStatePath = Utils::getFilePathWithoutExtension(gamePath) + ".state";
        cheatPath = gamePath + ".cht";
    }

    std::string Setting::getSaveStatePath(int idx) {
        if (idx == 0)
            return saveStatePath;
        return saveStatePath + std::to_string(idx);
    }
}

