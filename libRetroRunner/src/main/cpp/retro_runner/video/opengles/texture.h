//
// Created by aidoo on 2024/11/5.
//

#ifndef _TEXTURE_H
#define _TEXTURE_H


namespace libRetroRunner {
    class GLTextureObject {
    public :
        GLTextureObject() = default;

        ~GLTextureObject();

        void Create(unsigned int width, unsigned int height);

        void WriteTextureData(const void *data, unsigned int width, unsigned int height, int pixelFormat);

        void Destroy();

    public:
        inline unsigned int GetTexture() {
            return textureId_;
        }

        inline unsigned int GetWidth() {
            return texture_width_;
        }

        inline unsigned int GetHeight() {
            return texture_height_;
        }

    private:
        unsigned int texture_width_ = 0;
        unsigned int texture_height_ = 0;
        unsigned char *buffer_ = nullptr;
        unsigned int textureId_ = 0;
    };


}
#endif
