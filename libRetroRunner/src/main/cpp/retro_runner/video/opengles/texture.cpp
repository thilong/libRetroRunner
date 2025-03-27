//
// Created by Aidoo.TK on 2024/11/5.
//
#include "texture.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <libretro-common/include/libretro.h>
#include "../../types/log.h"

#define LOGD_TB(...) LOGD("[VIDEO] " __VA_ARGS__)
#define LOGW_TB(...) LOGW("[VIDEO] " __VA_ARGS__)
#define LOGE_TB(...) LOGE("[VIDEO] " __VA_ARGS__)
#define LOGI_TB(...) LOGI("[VIDEO] " __VA_ARGS__)

namespace libRetroRunner {

    void convertXRGB8888ToRGBA8888(const void *data, unsigned int width, unsigned int height, unsigned char *buffer) {
        const unsigned int *src = (const unsigned int *) data;
        unsigned char *dst = buffer;
        for (unsigned int i = 0; i < width * height; i++) {
            unsigned int pixel = src[i];
            *dst++ = (pixel >> 16) & 0xFF;
            *dst++ = (pixel >> 8) & 0xFF;
            *dst++ = pixel & 0xFF;
            *dst++ = (pixel >> 24) & 0xFF;
        }
    }

    void convertRGB565ToRGBA8888(const void *data, unsigned int width, unsigned int height, unsigned char *buffer) {
        //LOGD_TB("convertRGB565ToRGBA8888: %d x %d", width, height);

        const unsigned short *src = (const unsigned short *) data;
        unsigned char *dst = buffer;
        for (unsigned int heightIdx = 0; heightIdx < height; heightIdx++) {
            for (unsigned int rowIdx = 0; rowIdx < width; rowIdx++) {
                unsigned short pixel = src[heightIdx * width + rowIdx];
                dst = buffer + (heightIdx * width + rowIdx) * 4;
                //Red
                *dst = ((pixel & 0xF800) >> 11) << 3;
                dst++;
                //Green
                *dst = (((pixel & 0x7E0) >> 5) << 2);
                dst++;
                //Blue
                *dst = (((pixel & 0x1F)) << 3);
                dst++;
                *dst = 0xFF;
            }
        }
    }

    void convert0RGB1555ToRGBA8888(const void *data, unsigned int width, unsigned int height, unsigned char *buffer) {
        const unsigned short *src = (const unsigned short *) data;
        unsigned char *dst = buffer;
        for (unsigned int i = 0; i < width * height; i++) {
            unsigned short pixel = src[i];
            *dst++ = ((pixel >> 10) & 0x1F) << 3;
            *dst++ = ((pixel >> 5) & 0x1F) << 3;
            *dst++ = (pixel & 0x1F) << 3;
            *dst++ = (pixel >> 15) ? 0xFF : 0;
        }
    }

    GLTextureObject::~GLTextureObject() {
        Destroy();
    }

    void GLTextureObject::Create(unsigned int width, unsigned int height) {
        this->texture_width_ = width;
        this->texture_height_ = height;
        if (buffer_) {
            delete[] buffer_;
            buffer_ = nullptr;
        }
        bool linear = true;
        buffer_ = new unsigned char[width * height * 4];
        glGenTextures(1, &textureId_);

        glBindTexture(GL_TEXTURE_2D, textureId_);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8_OES, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);


        glBindTexture(GL_TEXTURE_2D, 0);

        LOGD_TB("texture created: %d, width:%u, height:%u", textureId_, width, height);
    }

    void GLTextureObject::WriteTextureData(const void *data, unsigned int width, unsigned int height, int pixelFormat) {
        if (width != this->texture_width_ || height != this->texture_height_) {
            if (buffer_) {
                delete[] buffer_;
                buffer_ = new unsigned char[width * height * 4];
                LOGW("[VIDEO] texture size changed: %d, width:%u, height:%u", textureId_, width, height);
            }
        }
        switch (pixelFormat) {
            case RETRO_PIXEL_FORMAT_XRGB8888:
                convertXRGB8888ToRGBA8888(data, width, height, buffer_);
                break;
            case RETRO_PIXEL_FORMAT_RGB565:
                convertRGB565ToRGBA8888(data, width, height, buffer_);
                break;
            case RETRO_PIXEL_FORMAT_0RGB1555:
                convert0RGB1555ToRGBA8888(data, width, height, buffer_);
                break;
        }

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, textureId_);

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer_);

        glBindTexture(GL_TEXTURE_2D, 0);

    }

    void GLTextureObject::Destroy() {
        if (buffer_) {
            delete[] buffer_;
            buffer_ = nullptr;
        }
        if (textureId_) {
            glDeleteTextures(1, &textureId_);
            textureId_ = 0;
        }
    }


}
