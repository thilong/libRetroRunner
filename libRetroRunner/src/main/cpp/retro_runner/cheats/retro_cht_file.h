//
// Created by Aidoo.TK on 2024/11/17.
//

#ifndef _RETRO_CONFIG_FILE_HPP
#define _RETRO_CONFIG_FILE_HPP

#include <string>
#include <map>
#include <memory>
#include "cheat_types.h"


namespace libRetroRunner {
    class RetroCheatFile {
    public:
        RetroCheatFile() {}

        static bool Load(const std::string &path, std::map<long, std::shared_ptr<Cheat>> &cheats);

        static bool Save(const std::string &path, std::map<long, std::shared_ptr<Cheat>> &cheats);

    private:

    };
}

#endif
