//
// Created by Aidoo.TK on 2024/11/18.
//
#include "retro_cht_file.h"
#include <fstream>

namespace libRetroRunner {

    bool RetroCheatFile::Load(const std::string &path, std::map<long, Cheat> &cheats) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        std::string line;
        int cheatIndex = 0;
        std::string cheatItemHead = "cheat" + std::to_string(cheatIndex) + "_";
        long id = time(nullptr);
        Cheat cheat;
        cheat.id = id;

        long addedId = -1;
        while (std::getline(file, line)) {
            if (line.empty() || line.length() < 10 || line.find("cheats =") == 0) continue;

            if (line.find(cheatItemHead) == std::string::npos) {
                cheats[cheat.id] = cheat;
                addedId = cheat.id;
                cheatIndex++;
                id++;

                cheat.id = id;
                cheatItemHead = "cheat" + std::to_string(cheatIndex) + "_";
            }

            if (line.find(cheatItemHead + "desc = ") == 0) {
                cheat.description = line.substr(cheatItemHead.length() + 6);
                //remove  "
                if (cheat.description.length() > 0 && cheat.description.find('"') != std::string::npos) {
                    int pos = cheat.description.find('"');
                    cheat.description = cheat.description.substr(pos+ 1, cheat.code.length() - (pos + 1));
                }
                if (cheat.description.length() > 0 && cheat.description[cheat.description.length() - 1] == '"') {
                    cheat.description = cheat.description.substr(0, cheat.description.length() - 1);
                }
            } else if (line.find(cheatItemHead + "code = ") == 0) {
                cheat.code = line.substr(cheatItemHead.length() + 6);
                //remove  "
                if (cheat.code.length() > 0 && cheat.code.find('"') != std::string::npos) {
                    int pos = cheat.code.find('"');
                    cheat.code = cheat.code.substr(pos + 1, cheat.code.length() - (pos + 1));
                }
                if (cheat.code.length() > 0 && cheat.code[cheat.code.length() - 1] == '"') {
                    cheat.code = cheat.code.substr(0, cheat.code.length() - 1);
                }
            } else if (line.find(cheatItemHead + "enable = ") == 0) {
                cheat.enabled = line.substr(cheatItemHead.length() + 9) == "true";
            }
        }
        if (addedId != cheat.id)
            cheats[cheat.id] = cheat;
        file.close();
        return true;
    }

    bool RetroCheatFile::Save(const std::string &path, std::map<long, Cheat> &cheats) {
        int cheatsCount = cheats.size();
        std::ofstream file(path, std::fstream::trunc | std::fstream::out);
        if (!file.is_open()) {
            return false;
        }
        file << "cheats = " << cheatsCount << std::endl;
        int idx = 0;
        for (auto pair: cheats) {
            Cheat &cheat = pair.second;
            file << "cheat" << idx << "_desc = \"" << cheat.description << "\"" << std::endl;
            file << "cheat" << idx << "_code = \"" << cheat.code << "\"" << std::endl;
            file << "cheat" << idx << "_enable = " << (cheat.enabled ? "true" : "false") << std::endl;
        }
        file.flush();
        file.close();
        return true;
    }
}