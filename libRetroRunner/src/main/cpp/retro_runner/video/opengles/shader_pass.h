//
// Created by aidoo on 2024/11/5.
//

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

        /*把一张纹理拉伸满绘制到目标上*/
        void FillTexture(GLuint textureId);

        /**
         * draw current framebuffer to screen
         * @param width  screen width
         * @param height  screen height
         * @param rotation  game video rotation
         * @see GLESVideoContext::game_video_ration_
         */
        void DrawOnScreen(int width, int height, unsigned int rotation = 0);

        /*把当前绑定的FBO绘制到文件中，dump*/
        void DrawToFile(const std::string &path);

    private:
        void drawTexture(GLuint textureId, int width = 0, int height = 0, unsigned rotation = 0);

    public:
        inline void SetPixelFormat(int format) {
            pixelFormat = format;
        }

        inline void SetHardwareAccelerated(bool accelerated) {
            hardwareAccelerated = accelerated;
        }

        //以下为shader相关
        inline GLuint GetProgramId() {
            return programId;
        }

        inline GLuint GetVertexShader() {
            return vertexShader;
        }

        inline GLuint GetFragmentShader() {
            return fragmentShader;
        }

        inline GLuint GetAttrPosition() {
            return attr_position;
        }

        inline GLuint GetAttrCoordinate() {
            return attr_coordinate;
        }

        inline GLuint GetAttrTexture() {
            return attr_texture;
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

        unsigned int pixelFormat;

        GLuint programId = 0;
        GLuint vertexShader = 0;
        GLuint fragmentShader = 0;

        GLuint attr_position;
        GLuint attr_coordinate;
        GLuint attr_texture;
        GLuint attr_flip_;

        GLuint vbo_position_ = 0;
        GLuint vbo_texture_coordinate_ = 0;

        bool hardwareAccelerated = false;
    };

}
#endif
