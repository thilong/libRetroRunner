//
// Created by Aidoo.TK on 2024/11/15.
//

#include "cheat_manager.h"
#include "../app/app_context.h"
#include "../core/core.h"
#include "../types/log.h"
#include "retro_cht_file.h"


#define LOGD_CHT(...) LOGD("[CHEAT] " __VA_ARGS__)
#define LOGW_CHT(...) LOGW("[CHEAT] " __VA_ARGS__)
#define LOGE_CHT(...) LOGE("[CHEAT] " __VA_ARGS__)
#define LOGI_CHT(...) LOGI("[CHEAT] " __VA_ARGS__)


namespace libRetroRunner {
    static long id_gen = time(nullptr);

    void __coreUpdateCheat(std::shared_ptr<Cheat> &cheat) {
        if (!cheat || cheat->code.empty()) return;
        auto app = AppContext::Current();
        if (!app) return;
        auto core = app->GetCore();
        if (!core || !core->retro_cheat_set) return;
        LOGW_CHT("updating cheat:%d, %s, %s, %d", cheat->index, cheat->code.c_str(), cheat->description.c_str(), cheat->enabled);
        core->retro_cheat_set(cheat->index, cheat->enabled, cheat->code.c_str());

    }

    void __coreRemoveAllCheat() {
        auto app = AppContext::Current();
        if (!app) return;
        auto core = app->GetCore();
        if (!core || core->retro_cheat_reset) return;
        core->retro_cheat_reset();
    }

    long __genId() {
        id_gen++;
        return id_gen;
    }

    CheatManager::CheatManager() {
        last_index_ = 0;
    }

    CheatManager::~CheatManager() {
        //clear all cheats
    }

    std::shared_ptr<Cheat> CheatManager::GetCheatAt(int index) {
        if (index < 0 || index >= cheats_.size()) {
            return nullptr;
        }

        auto pair = cheats_.begin();
        for (int i = 0; i < index; i++) {
            pair++;
        }
        return pair->second;
    }

    std::shared_ptr<Cheat> CheatManager::GetCheat(long id) {
        if (cheats_.count(id) == 0) {
            return nullptr;
        }
        return cheats_[id];
    }

    long CheatManager::AddCheat(const std::string &code, const std::string &description, bool enable) {
        long id = __genId();
        auto cheat = std::make_shared<Cheat>();
        cheat->id = id;
        cheat->code = code;
        cheat->description = description;
        cheat->enabled = enable;
        cheat->index = last_index_++;
        cheats_[id] = cheat;
        __coreUpdateCheat(cheat);
        return id;
    }

    void CheatManager::RemoveCheat(long id) {
        if (cheats_.count(id) == 0) return;
        auto cheat = cheats_[id];
        cheat->enabled = false;
        __coreUpdateCheat(cheat);
        cheats_.erase(id);
    }

    void CheatManager::UpdateCheatEnabled(long id, bool enable) {
        if (cheats_.count(id) > 0) {
            cheats_[id]->enabled = enable;
            __coreUpdateCheat(cheats_[id]);
        }
    }

    /*cheats that are added in core won't be remove but will be disabled.*/
    void CheatManager::RemoveAll() {
        __coreRemoveAllCheat();
        cheats_.clear();
    }

    int CheatManager::LoadFromFile(const std::string &path) {
        std::map<long, std::shared_ptr<Cheat>> tempCheats;

        if (!RetroCheatFile::Load(path, tempCheats)) {
            LOGE_CHT("load cheat file failed, from %s", path.c_str());
            return -1;
        }
        for (auto cheatPair: tempCheats) {
            auto cheat = cheatPair.second;
            cheat->index = last_index_++;
            __coreUpdateCheat(cheat);
            cheats_[cheatPair.first] = cheat;
        }
        return 0;
    }

    int CheatManager::SaveToFile(const std::string &path) {
        bool saveRet = RetroCheatFile::Save(path, cheats_);
        return saveRet ? 0 : -1;
    }

    int CheatManager::LoadFromSetting() {
        /*
        auto app = AppContext::Current();
        auto setting = app->GetSetting();
        if (!setting) {
            LOGE_CHT("load cheat file failed, no file path is set yet.");
            return -1;
        }
        return LoadFromFile(setting->cheatPath);
         */
        //TODO: 需要实现
        return -1;
    }


}
