/*
 * Created by Aidoo.TK on 2024/11/5.
 *
 * we create framebuffer to each shader pass.
 * draw the texture of game to pass 0, then each pass draw the it's texture to the framebuffer
 * of next pass. After done all pass drawing. We draw the final one to the screen.
*/

#ifndef _SHADER_PASS_H
#define _SHADER_PASS_H

#include <GLES2/gl2.h>
#include <string>
#include "frame_buffer_object.h"

namespace libRetroRunner {
    class GLShaderPass {
    public:

        GLShaderPass(const char *vertexShaderCode, char *fragmentShaderCode);

        ~GLShaderPass();

        void Destroy();

        void CreateFrameBuffer(int width, int height, bool linear, bool includeDepth, bool includeStencil);

        /*draw the texture with id of 'textureId' to current framebuffer.*/
        void FillTexture(GLuint textureId);

        /**
         * draw current framebuffer to screen
         * @param width  screen width
         * @param height  screen height
         * @param rotation  game video rotation
         * @see GLESVideoContext::game_video_ration_
         */
        void DrawOnScreen(int width, int height, unsigned int rotation = 0);

        /*draw the texture of current framebuffer to file.*/
        void DrawToFile(const std::string &path);

    private:
        void drawTexture(GLuint textureId, int width = 0, int height = 0, unsigned rotation = 0);

    public:
        inline void SetPixelFormat(int format) {
            pixelFormat = format;
        }

        inline void SetHardwareAccelerated(bool accelerated) {
            hardware_accelerated_ = accelerated;
        }

        //以下为shader相关
        inline GLuint GetProgramId() {
            return program_id_;
        }

        inline GLuint GetVertexShader() {
            return vertex_shader_;
        }

        inline GLuint GetFragmentShader() {
            return fragment_shader_;
        }

        inline GLuint GetAttrPosition() {
            return attr_position_;
        }

        inline GLuint GetAttrCoordinate() {
            return attr_coordinate_;
        }

        inline GLuint GetAttrTexture() {
            return attr_texture_;
        }
        //以下为framebuffer相关

        inline GLuint GetFrameBuffer() {
            if (frameBuffer == nullptr) {
                return 0;
            }
            return frameBuffer->GetFrameBuffer();
        }

        inline GLuint GetTexture() {
            if (frameBuffer == nullptr) {
                return 0;
            }
            return frameBuffer->GetTexture();
        }

        inline unsigned GetWidth() {
            if (frameBuffer == nullptr) {
                return 0;
            }
            return frameBuffer->GetWidth();
        }

        inline unsigned GetHeight() {
            if (frameBuffer == nullptr) {
                return 0;
            }
            return frameBuffer->GetHeight();
        }

    private:
        std::unique_ptr<GLFrameBufferObject> frameBuffer;

        /** core pixel format,
         * only used to detect if the core pixel format is set.
         * */
        unsigned int pixelFormat;

        GLuint program_id_ = 0;
        GLuint vertex_shader_ = 0;
        GLuint fragment_shader_ = 0;

        GLuint attr_position_;
        GLuint attr_coordinate_;
        GLuint attr_texture_;
        GLuint attr_flip_;

        GLuint vbo_position_ = 0;
        GLuint vbo_texture_coordinate_ = 0;

        bool hardware_accelerated_ = false;
    };

}
#endif
