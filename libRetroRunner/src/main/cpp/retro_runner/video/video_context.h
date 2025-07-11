//
// Created by Aidoo.TK on 2024/11/4.
//

#ifndef _VIDEO_CONTEXT_H
#define _VIDEO_CONTEXT_H

#include <memory>
#include <string>
#include <jni.h>

#include <retro_runner/runtime_contexts/game_context.h>

namespace libRetroRunner {


    class VideoContext {
    public:
        static std::shared_ptr<VideoContext> Create(std::string &driver, int retroHWContextType);

        VideoContext();

        virtual ~VideoContext();

        virtual void Destroy() = 0;

        virtual bool Load() = 0;

        /*unload video output, keep context.*/
        virtual void Unload() = 0;

        /*显示环境被暂停了，无法再进行显示，比如android的surface暂时清空了
         * 暂时视频组件的动作，等待UnLoad命令*/
        virtual void SetWindowPaused() = 0;

        virtual void UpdateVideoSize(unsigned width, unsigned height) = 0;

        /* prepare video context for every frame before emu-step.*/
        virtual void Prepare() = 0;

        virtual void DrawFrame() = 0;

        virtual void OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) = 0;

        virtual unsigned int GetCurrentFramebuffer() { return 0; }

        /* dump video frame into a file, may fail, this should run on emu thread. */
        virtual bool TakeScreenshot(const std::string &path) = 0;

        /** provide hardware render interface, return false if no interface .  */
        virtual bool getRetroHardwareRenderInterface(void **) { return false; };

        /* set the path to store the next screenshot, when finish dumping, path will be set to empty. */
        void SetNextScreenshotStorePath(std::string &path);

        /*set if video output is enabled.*/
        void SetEnabled(bool flag);

        void SetGameContext(std::shared_ptr<GameRuntimeContext> &ctx);

    protected:
        bool enabled_;
        std::string next_screenshot_store_path_;
        std::weak_ptr<GameRuntimeContext> game_runtime_ctx_;
    };
}
#endif
