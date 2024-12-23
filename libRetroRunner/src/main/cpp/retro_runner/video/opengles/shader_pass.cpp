//
// Created by Aidoo.TK on 2024/11/5.
//

#include <android/bitmap.h>
#include <libretro-common/include/libretro.h>
#include "shader_pass.h"
#include "shaders.h"
#include "../../types/log.h"


#define LOGD_SP(...) LOGD("[VIDEO]:[SHADERPASS] " __VA_ARGS__)
#define LOGW_SP(...) LOGW("[VIDEO]:[SHADERPASS] " __VA_ARGS__)
#define LOGE_SP(...) LOGE("[VIDEO]:[SHADERPASS] " __VA_ARGS__)
#define LOGI_SP(...) LOGI("[VIDEO]:[SHADERPASS] " __VA_ARGS__)
namespace libRetroRunner {
#ifdef ANDROID
    extern "C" JavaVM *gVm;

    void savePixelsToFile(GLubyte *flippedPixels, int width, int height, const std::string &path) {
        JNIEnv *env = nullptr;
        gVm->AttachCurrentThread(&env, nullptr);

        // 3. 创建 Bitmap 对象
        jclass bitmapClass = env->FindClass("android/graphics/Bitmap");

        // 获取 Bitmap.Config.ARGB_8888 配置的值
        jclass configClass = env->FindClass("android/graphics/Bitmap$Config");
        jfieldID argb8888Field = env->GetStaticFieldID(configClass, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
        jobject argb8888Config = env->GetStaticObjectField(configClass, argb8888Field);

        // 获取 Bitmap.createBitmap 方法的签名：createBitmap(int width, int height, Bitmap.Config config)
        jmethodID createBitmapMethod = env->GetStaticMethodID(bitmapClass, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");

        if (createBitmapMethod == nullptr) {
            free(flippedPixels);
            return; // 方法未找到，退出
        }

        // 调用 createBitmap 创建 Bitmap 对象
        jobject bitmap = env->CallStaticObjectMethod(bitmapClass, createBitmapMethod, width, height, argb8888Config);
        if (bitmap == nullptr) {
            free(flippedPixels);
            return; // 创建 Bitmap 失败
        }

        // 4. 将像素数据填充到 Bitmap 对象中
        AndroidBitmapInfo bitmapInfo;
        AndroidBitmap_getInfo(env, bitmap, &bitmapInfo);

        void *bitmapPixels;
        AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels);

        // 拷贝像素数据
        memcpy(bitmapPixels, flippedPixels, 4 * width * height);

        AndroidBitmap_unlockPixels(env, bitmap); // 解锁 Bitmap
        jstring filePath = env->NewStringUTF(path.c_str());
        // 5. 保存 Bitmap 为 PNG 文件
        jclass fileClass = env->FindClass("java/io/File");
        jmethodID fileConstructor = env->GetMethodID(fileClass, "<init>", "(Ljava/lang/String;)V");
        jobject fileObj = env->NewObject(fileClass, fileConstructor, filePath);

        jclass fileOutputStreamClass = env->FindClass("java/io/FileOutputStream");
        jmethodID fileOutputStreamConstructor = env->GetMethodID(fileOutputStreamClass, "<init>", "(Ljava/io/File;)V");
        jobject fileOutputStream = env->NewObject(fileOutputStreamClass, fileOutputStreamConstructor, fileObj);

        jclass bitmapClass2 = env->FindClass("android/graphics/Bitmap");
        jmethodID compressMethod = env->GetMethodID(bitmapClass2, "compress", "(Landroid/graphics/Bitmap$CompressFormat;ILjava/io/OutputStream;)Z");

        jclass compressFormatClass = env->FindClass("android/graphics/Bitmap$CompressFormat");
        jfieldID pngField = env->GetStaticFieldID(compressFormatClass, "PNG", "Landroid/graphics/Bitmap$CompressFormat;");
        jobject pngFormat = env->GetStaticObjectField(compressFormatClass, pngField);

        jboolean success = env->CallBooleanMethod(bitmap, compressMethod, pngFormat, 100, fileOutputStream);
        if (!success) {
            LOGE_SP("Failed to save bitmap to file");
        }

        // 清理资源
        env->DeleteLocalRef(fileOutputStream);
        env->DeleteLocalRef(fileObj);
        env->DeleteLocalRef(compressFormatClass);
        env->DeleteLocalRef(bitmapClass2);
        gVm->DetachCurrentThread();
    }

#else
    void savePixelsToFile(GLubyte *flippedPixels, int width, int height, const std::string &path){
    }
#endif
}

//静态变量与公共方法
namespace libRetroRunner {

    GLfloat glPositionVBOData[12] = {
            -1.0F, -1.0F,  //左下
            -1.0F, +1.0F,  //左上
            +1.0F, +1.0F,  //右上
            +1.0F, -1.0F,  //右下

            -1.0F, -1.0F,  //左下
            -1.0F, +1.0F,  //左上
    };

    GLfloat glTextureCoordVBOData[48] = {
            0.0F, 0.0F,  //左下
            0.0F, 1.0F, //左上
            1.0F, 1.0F, //右上
            1.0F, 0.0F, //右下

            0.0F, 0.0F, //左下
            0.0F, 1.0F, //左上
            1.0F, 1.0F, //右上
            1.0F, 0.0F, //右下

            0.0F, 0.0F,  //左下
            0.0F, 1.0F, //左上
            1.0F, 1.0F, //右上
            1.0F, 0.0F, //右下

            0.0F, 0.0F,  //左下
            0.0F, 1.0F, //左上
            1.0F, 1.0F, //右上
            1.0F, 0.0F, //右下

            0.0F, 0.0F,  //左下
            0.0F, 1.0F, //左上
            1.0F, 1.0F, //右上
            1.0F, 0.0F, //右下
    };


    static GLuint _loadShader(GLenum shaderType, const char *source) {
        GLuint shader = glCreateShader(shaderType);
        if (shader) {
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);
            GLint compiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (!compiled) {
                GLint infoLen = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
                if (infoLen) {
                    char *buf = (char *) malloc(infoLen);
                    if (buf) {
                        glGetShaderInfoLog(shader, infoLen, nullptr, buf);
                        LOGE_SP("Could not compile shader %d:\n%s\n",
                                shaderType, buf);
                        free(buf);
                    }
                    glDeleteShader(shader);
                    shader = 0;
                }
            }
        }
        return shader;
    }

    static GLuint _createVBO(GLfloat *bufferData, GLsizeiptr size) {
        GLuint buffer = 0;
        glGenBuffers(1, &buffer);
        if (buffer == 0) {
            return 0;
        }
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size, bufferData, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return buffer;
    }
}

//GLShaderPass定义
namespace libRetroRunner {
    GLShaderPass::GLShaderPass(const char *vertexShaderCode, char *fragmentShaderCode) {
        pixelFormat = RETRO_PIXEL_FORMAT_UNKNOWN;
        const char *finalVertexShaderCode = vertexShaderCode ? vertexShaderCode : default_vertex_shader.c_str();
        const char *finalFragmentShaderCode = fragmentShaderCode ? fragmentShaderCode : default_fragment_shader.c_str();

        vertex_shader_ = _loadShader(GL_VERTEX_SHADER, finalVertexShaderCode);
        if (!vertex_shader_) {
            return;
        }

        fragment_shader_ = _loadShader(GL_FRAGMENT_SHADER, finalFragmentShaderCode);
        if (!fragment_shader_) {
            return;
        }

        GLuint program = glCreateProgram();
        if (program) {
            glAttachShader(program, vertex_shader_);
            glAttachShader(program, fragment_shader_);
            glLinkProgram(program);
            GLint linkStatus = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
            if (linkStatus != GL_TRUE) {
                GLint bufLength = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
                if (bufLength) {
                    char *buf = (char *) malloc(bufLength);
                    if (buf) {
                        glGetProgramInfoLog(program, bufLength, nullptr, buf);
                        LOGE_SP("Could not link program:\n%s\n", buf);
                        free(buf);
                    }
                }
                glDeleteProgram(program);
                program = 0;
            }
        }

        if (program) {
            program_id_ = program;
            attr_position_ = glGetAttribLocation(program, "a_position");
            attr_coordinate_ = glGetAttribLocation(program, "a_texCoord");
            attr_texture_ = glGetUniformLocation(program, "u_texture");
            attr_flip_ = glGetUniformLocation(program, "u_flipVertical");
            LOGD_SP("shader pass created, program id: %d", program_id_);
        }
    }

    GLShaderPass::~GLShaderPass() {
        Destroy();
    }

    void GLShaderPass::Destroy() {
        if (vbo_position_) {
            glDeleteBuffers(1, &vbo_position_);
            vbo_position_ = 0;
        }
        if (vbo_texture_coordinate_) {
            glDeleteBuffers(1, &vbo_texture_coordinate_);
            vbo_texture_coordinate_ = 0;
        }
        if (program_id_) {
            glDeleteProgram(program_id_);
            program_id_ = 0;
        }
        if (vertex_shader_) {
            glDeleteShader(vertex_shader_);
            vertex_shader_ = 0;
        }
        if (fragment_shader_) {
            glDeleteShader(fragment_shader_);
            fragment_shader_ = 0;
        }

    }

    void GLShaderPass::CreateFrameBuffer(int width, int height, bool linear, bool includeDepth, bool includeStencil) {
        if (pixelFormat == RETRO_PIXEL_FORMAT_UNKNOWN) {
            LOGE_SP("pixel format is unknow, can't create frame buffer for %d", pixelFormat);
            return;
        }
        if (frameBuffer != nullptr && width == frameBuffer->GetWidth() && height == frameBuffer->GetHeight()) {
            LOGW_SP("frame buffer size not change, reuse it.");
            return;
        }
        frameBuffer = std::make_unique<GLFrameBufferObject>();
        frameBuffer->SetSize(width, height);
        frameBuffer->SetLinear(linear);
        frameBuffer->Create(includeDepth, includeStencil);

        if (!vbo_position_) {
            vbo_position_ = _createVBO(glPositionVBOData, sizeof(glPositionVBOData));
        }
        if (!vbo_position_) {
            LOGE_SP("create position vbo failed.");
        }

        if (!vbo_texture_coordinate_) {
            vbo_texture_coordinate_ = _createVBO(glTextureCoordVBOData, sizeof(glTextureCoordVBOData));
        }
        if (!vbo_texture_coordinate_) {
            LOGE_SP("create position vbo failed.");
        }
        GL_CHECK("GLShaderPass::CreateFrameBuffer");
    }

    void GLShaderPass::DrawOnScreen(int width, int height, unsigned int rotation) {
        drawTexture(0, width, height, rotation);
    }

    void GLShaderPass::FillTexture(GLuint textureId) {
        drawTexture(textureId);
    }

    //when textureId is not 0, means draw the textureId to framebuffer of this pass.
    //otherwise, draw the texture of current pass to screen.
    //we should deal the following attributes: size, rotation, position...
    void GLShaderPass::drawTexture(GLuint textureId, int width, int height, unsigned int rotation) {
        bool renderToScreen = textureId == 0;
        if (renderToScreen) {
            glViewport(0, 0, width, height);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        } else {
            glViewport(0, 0, frameBuffer->GetWidth(), frameBuffer->GetHeight());
            glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer->GetFrameBuffer());
        }

        glUseProgram(program_id_);

        //position
        glBindBuffer(GL_ARRAY_BUFFER, vbo_position_);
        glEnableVertexAttribArray(attr_position_);
        glVertexAttribPointer(attr_position_, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

        //vertex
        glBindBuffer(GL_ARRAY_BUFFER, vbo_texture_coordinate_);
        glEnableVertexAttribArray(attr_coordinate_);
        if (renderToScreen) {
            glVertexAttribPointer(attr_coordinate_, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void *) (2 * rotation * sizeof(GLfloat)));
        } else
            glVertexAttribPointer(attr_coordinate_, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void *) 0);

        //flip
        glEnableVertexAttribArray(attr_flip_);
        if (renderToScreen && !hardware_accelerated_) {
            glUniform1i(attr_flip_, true);
        } else {
            glUniform1i(attr_flip_, false);
        }


        glActiveTexture(GL_TEXTURE0);
        if (renderToScreen) {
            glBindTexture(GL_TEXTURE_2D, frameBuffer->GetTexture());
            glUniform1i(attr_texture_, 0);
        } else {
            glBindTexture(GL_TEXTURE_2D, textureId);
            glUniform1i(attr_texture_, 0);
        }


        glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);

        glDisableVertexAttribArray(attr_position_);
        glDisableVertexAttribArray(attr_coordinate_);

        glUseProgram(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GLShaderPass::DrawToFile(const std::string &path) {
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer->GetFrameBuffer());
        LOGW_SP("dump framebuffer to file: %s", path.c_str());
        int width = frameBuffer->GetWidth();
        int height = frameBuffer->GetHeight();
        GLubyte *pixels = (GLubyte *) malloc(width * height * 4);
        if (pixels == nullptr) {
            LOGE_SP("malloc buffer for dump failed.");
            return;
        }

        GLubyte *flippedPixels = (GLubyte *) malloc(width * height * 4);
        if (flippedPixels == nullptr) {
            LOGE_SP("malloc buffer2 for dump failed.");
            free(pixels);
            return;
        }
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        /* we should flip the texture in software rendering.*/
        if (!hardware_accelerated_) {
            memcpy(flippedPixels, pixels, width * height * 4);
        } else {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int srcIndex = (y * width + x) * 4;
                    int destIndex = ((height - 1 - y) * width + x) * 4;
                    flippedPixels[destIndex] = pixels[srcIndex];
                    flippedPixels[destIndex + 1] = pixels[srcIndex + 1];
                    flippedPixels[destIndex + 2] = pixels[srcIndex + 2];
                    flippedPixels[destIndex + 3] = pixels[srcIndex + 3];
                }
            }
        }

        savePixelsToFile(flippedPixels, width, height, path);
        free(pixels);
        free(flippedPixels);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }


}

