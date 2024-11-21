//
// Created by aidoo on 2024/11/13.
//

#ifndef _SETTING_H
#define _SETTING_H

#include <string>

namespace libRetroRunner {
    class Setting {
    public:
        Setting();

        ~Setting();

        /**
         * 根据游戏路径和存档路径来初始化相关设置
         * @param gamePath  游戏路径
         * @param savePath  存档路径
         */
        void InitWithPaths(const std::string &gamePath, const std::string &savePath);

        std::string getSaveStatePath(int idx = 0);

    public:

        /*声音使用低延迟传输*/
        bool lowLatency = true;

        /*
         * 游戏存档的保存路径
         * 游戏在开始的时候需要自动加载这个路径的文件，如果不加载，游戏则无法读取存档
         * */
        std::string saveRamPath;

        /*
         * 游戏即时存档保存路径，即游戏的快照
         * 编号规则为 {路径}.{0-9}，例如 /sdcard/retroarch/game.zip.0
         * 编号为0的是自动保存路径，其他编号为手动保存路径
         */
        std::string saveStatePath;

        std::string cheatPath;
    };

}


#endif
