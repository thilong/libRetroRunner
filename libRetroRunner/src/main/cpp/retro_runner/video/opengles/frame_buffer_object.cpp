//
// Created by Aidoo.TK on 2024/11/5.
//
#include "frame_buffer_object.h"
#include <GLES2/gl2.h>
#include <stdexcept>
#include "../../types/log.h"

#define LOGD_FBO(...) LOGD("[VIDEO] " __VA_ARGS__)
#define LOGW_FBO(...) LOGW("[VIDEO] " __VA_ARGS__)
#define LOGE_FBO(...) LOGE("[VIDEO] " __VA_ARGS__)
#define LOGI_FBO(...) LOGI("[VIDEO] " __VA_ARGS__)


namespace libRetroRunner {
    GLFrameBufferObject::GLFrameBufferObject() {
        frame_buffer = 0;
        texture_id = 0;
        depth = 0;
        width = 0;
        height = 0;
    }

    GLFrameBufferObject::~GLFrameBufferObject() {
        Destroy();
    }

    void GLFrameBufferObject::Create(bool includeDepth, bool includeStencil) {
        Destroy();
        depth = includeDepth;

        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &frame_buffer);
        if (depth) {
            glGenRenderbuffers(1, &depth_buffer);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
        //create texture for fbo
        glGenTextures(1, &texture_id);

        glBindTexture(GL_TEXTURE_2D, texture_id);

        //glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8_OES, width, height);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8_OES, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id, 0);


        if (depth) {

            // 2: GL_DEPTH24_STENCIL8_OES  , 3: GL_DEPTH24_STENCIL8
            glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
            glRenderbufferStorage(
                    GL_RENDERBUFFER,
                    includeStencil ? GL_DEPTH24_STENCIL8_OES : GL_DEPTH_COMPONENT16,
                    width,
                    height
            );

            glFramebufferRenderbuffer(
                    GL_FRAMEBUFFER,
                    GL_DEPTH_ATTACHMENT,
                    GL_RENDERBUFFER,
                    depth_buffer
            );
            if (includeStencil) {
                glFramebufferRenderbuffer(
                        GL_FRAMEBUFFER,
                        GL_STENCIL_ATTACHMENT,
                        GL_RENDERBUFFER,
                        depth_buffer
                );
            }
        }

        int frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (frameBufferStatus != GL_FRAMEBUFFER_COMPLETE) {
            LOGE_FBO("Error creating framebuffer %d not complete, status: %d, error: %d, %s", frame_buffer, frameBufferStatus, glGetError(), glGetString(glGetError()));
            throw std::runtime_error("Cannot create framebuffer");
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        LOGD_FBO("Frame buffer created, id:%d, size:%d x %d", frame_buffer, width, height);
    }

    void GLFrameBufferObject::Destroy() {
        if (depth_buffer > 0) {
            glDeleteRenderbuffers(1, &depth_buffer);
            depth_buffer = 0;
            depth = false;
        }
        if (texture_id > 0) {
            glDeleteTextures(1, &texture_id);
            texture_id = 0;
        }
        if (frame_buffer > 0) {
            glDeleteFramebuffers(1, &frame_buffer);
            frame_buffer = 0;
        }
    }

    void GLFrameBufferObject::SetSize(int w, int h) {
        width = w;
        height = h;
    }

    void GLFrameBufferObject::SetLinear(bool flag) {
        this->linear = flag;
    }

    GLuint GLFrameBufferObject::GetFrameBuffer() {
        return frame_buffer;
    }

    GLuint GLFrameBufferObject::GetTexture() {
        return texture_id;
    }

    void GLFrameBufferObject::SetPixelFormat(unsigned int format) {
        this->pixel_format = format;
    }
}

