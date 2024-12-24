//
// Created by Aidoo.TK on 2024/11/5.
//

#ifndef _FRAME_BUFFER_OBJECT_H
#define _FRAME_BUFFER_OBJECT_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


namespace libRetroRunner {
    class GLFrameBufferObject {
    public:
        GLFrameBufferObject();

        ~GLFrameBufferObject();

        void SetSize(int w, int h);

        void SetLinear(bool flag);

        void SetPixelFormat(unsigned int format);

        void Create(bool includeDepth, bool includeStencil);

        void Destroy();

        GLuint GetFrameBuffer();

        GLuint GetTexture();

    public:
        inline unsigned GetWidth() const {
            return width;
        }

        inline unsigned GetHeight() const {
            return height;
        }

    private:
        /** not used, we use RGBA8888 for all platforms.*/
        unsigned int pixel_format;

        bool linear;
        bool depth;

        GLuint frame_buffer = 0;
        GLuint texture_id = 0;
        GLuint depth_buffer = 0;
        unsigned width = 0;
        unsigned height = 0;

    };
}

#endif
