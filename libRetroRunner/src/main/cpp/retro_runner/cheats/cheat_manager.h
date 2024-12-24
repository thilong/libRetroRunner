//
// Created by Aidoo.TK on 2024/11/15.
//

#ifndef _CHEAT_H
#define _CHEAT_H

#include <string>
#include <map>
#include "cheat_types.h"

namespace libRetroRunner {


    class CheatManager {
    public:
        CheatManager();

        ~CheatManager();

        int LoadFromSetting();

        int LoadFromFile(const std::string &path);

        int SaveToFile(const std::string &path);

        int GetCheatCount() { return cheats_.size(); }

        std::shared_ptr<Cheat> GetCheatAt(int index);

        std::shared_ptr<Cheat> GetCheat(long id);

        long AddCheat(const std::string &code, const std::string &description, bool enable);

        void RemoveCheat(long id);

        void RemoveAll();

        void UpdateCheatEnabled(long id, bool enable);

    private:
        int last_index_ = 0;
        std::map<long, std::shared_ptr<Cheat>> cheats_;
    };
}


#endif
