//
// Created by Aidoo.TK on 2024/11/18.
//
#include "retro_cht_file.h"
#include <fstream>
#include <regex>

namespace libRetroRunner {

    extern long __genId();

    bool RetroCheatFile::Load(const std::string &path, std::map<long, std::shared_ptr<Cheat>> &cheats) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        int total_cheats = 0;
        std::string line;
        //step 1: check cheat count
        std::string cheatCountHead = "cheats = ";
        while (std::getline(file, line)) {
            if (line.find(cheatCountHead) == 0) {
                int count = std::stoi(line.substr(cheatCountHead.length()));
                file.close();
                if (count <= 0) {
                    return false;
                }
                total_cheats = count;
                break;
            }
        }
        std::vector<std::shared_ptr<Cheat>> tempCheats(total_cheats);

        //step 2: load cheats
        file.seekg(0, std::ios::beg);
        while (std::getline(file, line)) {
            std::regex cheatPattern(R"(cheat(\d+)_(\w+) = (.+))");
            std::smatch match;
            if (std::regex_search(line, match, cheatPattern)) {
                int index = std::stoi(match[1]);
                std::string property = match[2];
                std::string value = match[3];
                //remove " at start in value
                if (value[0] == '"') {
                    value = value.substr(1, value.length() - 2);
                }
                //remove " at end in value
                if (value.length() > 0 && value[value.length() - 1] == '"') {
                    value = value.substr(0, value.length() - 1);
                }


                if (index >= total_cheats) {
                    break;
                }
                std::shared_ptr<Cheat> cheat = tempCheats[index];
                if (!cheat) {
                    cheat = std::make_shared<Cheat>();
                    tempCheats[index] = cheat;
                }
                cheat->index = index;
                if (property == "desc") {
                    cheat->description = value;
                } else if (property == "code") {
                    cheat->code = value;
                } else if (property == "enable") {
                    cheat->enabled = (value == "true");
                }
            }
        }
        file.close();

        //step 3: move tempCheats to cheats
        for (auto cheat: tempCheats) {
            if (cheat) {
                long id = __genId();
                cheat->id = id;
                cheats[id] = cheat;
            }
        }

        return true;
    }

    bool RetroCheatFile::Save(const std::string &path, std::map<long, std::shared_ptr<Cheat>> &cheats) {
        int cheatsCount = cheats.size();
        std::ofstream file(path, std::fstream::trunc | std::fstream::out);
        if (!file.is_open()) {
            return false;
        }
        file << "cheats = " << cheatsCount << std::endl;
        int idx = 0;
        for (auto pair: cheats) {
            auto cheat = pair.second;
            file << "cheat" << idx << "_desc = \"" << cheat->description << "\"" << std::endl;
            file << "cheat" << idx << "_code = \"" << cheat->code << "\"" << std::endl;
            file << "cheat" << idx << "_enable = " << (cheat->enabled ? "true" : "false") << std::endl;
        }
        file.flush();
        file.close();
        return true;
    }
}