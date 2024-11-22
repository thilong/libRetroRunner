//
// Created by aidoo on 2024/11/4.
//

#ifndef _VIDEO_CONTEXT_H
#define _VIDEO_CONTEXT_H

#include <memory>
#include <string>
#include <jni.h>


namespace libRetroRunner {


    class VideoContext {
    public:
        static std::shared_ptr<VideoContext> Create(std::string &driver);

        VideoContext();

        virtual ~VideoContext();

        virtual bool Init() = 0;

        virtual void Destroy() = 0;

        /* prepare video context for every frame before emu-step.*/
        virtual void Prepare() = 0;

        virtual void DrawFrame() = 0;

        virtual void OnNewFrame(const void *data, unsigned int width, unsigned int height, size_t pitch) = 0;

        virtual void OnGameGeometryChanged() = 0;

        virtual void SetSurface(int argc, void **argV) = 0;

        virtual void SetSurfaceSize(unsigned width, unsigned height) = 0;

        virtual unsigned int GetCurrentFramebuffer() { return 0; }

        virtual void SetCoreOutputPixelFormat(int format) = 0;

        /* set the path to store the next screenshot, when finish dumping, path will be set to empty. */
        void SetNextScreenshotStorePath(std::string &path);

        /* set the game geometry changed flag, this is notified by core.
         * when a new video context is created, the flag is set to true.
         * */
        void SetGameGeometryChanged(bool changed) {
            game_geometry_changed_ = changed;
        }

    protected:

        std::string next_screenshot_store_path_;
        bool game_geometry_changed_ ;
    };
}
#endif
