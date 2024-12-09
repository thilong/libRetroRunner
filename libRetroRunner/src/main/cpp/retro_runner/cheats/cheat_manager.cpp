//
// Created by aidoo on 2024/11/15.
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

    void __coreUpdateCheat(Cheat &cheat) {
        if (cheat.code.empty()) return;
        auto app = AppContext::Current();
        if (!app) return;
        auto core = app->GetCore();
        if (!core || !core->retro_cheat_set) return;
        LOGW_CHT("try update cheat:%d, %s, %s, %d", cheat.index, cheat.code.c_str(), cheat.description.c_str(), cheat.enabled);
        core->retro_cheat_set(cheat.index, cheat.enabled, cheat.code.c_str());

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
        badCheat.id = 0;
        badCheat.enabled = false;
        badCheat.index = -1;
    }

    CheatManager::~CheatManager() {

    }

    Cheat &CheatManager::GetCheatAt(int index) {
        if (index < 0 || index >= cheats.size()) {
            return badCheat;
        }
        auto pair = cheats.begin();
        for (int i = 0; i < index; i++) {
            pair++;
        }
        return pair->second;
    }

    Cheat &CheatManager::GetCheat(long id) {
        if (cheats.count(id) == 0) {
            return badCheat;
        }
        return cheats[id];
    }

    long CheatManager::AddCheat(const std::string &code, const std::string &description, bool enable) {
        long id = __genId();
        Cheat cheat;
        cheat.id = id;
        cheat.code = code;
        cheat.description = description;
        cheat.enabled = enable;
        cheat.index = lastIndex++;
        cheats[id] = cheat;
        __coreUpdateCheat(cheat);
        return id;
    }

    void CheatManager::RemoveCheat(long id) {
        if (cheats.count(id) == 0) return;
        Cheat &cheat = cheats[id];
        cheat.enabled = false;
        __coreUpdateCheat(cheat);
        cheats.erase(id);
    }

    void CheatManager::UpdateCheatEnabled(long id, bool enable) {
        if (cheats.count(id) > 0) {
            cheats[id].enabled = enable;
            __coreUpdateCheat(cheats[id]);
        }
    }

    /*已经添加到核心中的条目并不会被删除，只会被禁用*/
    void CheatManager::RemoveAll() {
        __coreRemoveAllCheat();
        cheats.clear();
    }

    int CheatManager::LoadFromFile(const std::string &path) {
        if (!RetroCheatFile::Load(path, cheats)) {
            LOGE_CHT("load cheat file failed, from %s", path.c_str());
            return -1;
        }
        for (auto cheat: cheats) {
            __coreUpdateCheat(cheat.second);
        }
        return 0;
    }

    int CheatManager::SaveToFile(const std::string &path) {
        bool saveRet = RetroCheatFile::Save(path, cheats);
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
