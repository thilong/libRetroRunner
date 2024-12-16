#include <iostream>
#include <fstream>
#include <unistd.h>

#include "utils.h"

namespace libRetroRunner {

    std::vector<unsigned char> Utils::readFileAsBytes(const std::string &filePath) {
        FILE *file = fopen(filePath.c_str(), "rb");
        if (file == nullptr) {
            return {};
        }
        size_t size = getFileSize(file);

        std::vector<unsigned char> ret(size);
        fread(&(ret[0]), sizeof(unsigned char), size, file);
        fclose(file);
        return ret;
    }

    std::vector<unsigned char> Utils::readFileAsBytes(const int fileDescriptor) {
        FILE *file = fdopen(fileDescriptor, "r");
        size_t size = getFileSize(file);

        std::vector<unsigned char> ret(size);
        fread(&(ret[0]), sizeof(unsigned char), size, file);
        fclose(file);
        return ret;
    }

    size_t Utils::getFileSize(FILE *file) {
        fseek(file, 0, SEEK_SET);
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);
        return size;
    }

    int Utils::writeBytesToFile(const std::string &filePath, const char *data, size_t size) {
        FILE *file = fopen(filePath.c_str(), "wb");
        if (file == nullptr) {
            return -1;
        }
        size_t wrote = fwrite(data, sizeof(unsigned char), size, file);
        fclose(file);
        return (int) wrote;
    }

    std::string Utils::getFilePathWithoutExtension(const std::string &filePath) {
        size_t pos = filePath.find_last_of('.');
        if (pos != std::string::npos) {
            return filePath.substr(0, pos);
        }
        return filePath;
    }

}