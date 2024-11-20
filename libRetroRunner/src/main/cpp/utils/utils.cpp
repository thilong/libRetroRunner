#include <iostream>
#include <fstream>
#include <unistd.h>

#include "utils.h"

namespace libRetroRunner {

    Utils::ReadResult Utils::readFileAsBytes(const std::string &filePath) {
        FILE *file = fopen(filePath.c_str(), "rb");
        if (file == nullptr) {
            return ReadResult{0, nullptr};
        }
        size_t size = getFileSize(file);

        char *bytes = new char[size];
        fread(bytes, sizeof(char), size, file);
        fclose(file);
        return ReadResult{size, bytes};
    }

    Utils::ReadResult Utils::readFileAsBytes(const int fileDescriptor) {
        FILE *file = fdopen(fileDescriptor, "r");
        size_t size = getFileSize(file);

        char *bytes = new char[size];
        fread(bytes, sizeof(char), size, file);
        close(fileDescriptor);
        return ReadResult{size, bytes};
    }

    size_t Utils::getFileSize(FILE *file) {
        fseek(file, 0, SEEK_SET);
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);
        return size;
    }

    const char *Utils::cloneToCString(const std::string &input) {
        char *result = new char[input.length() + 1];
        std::strcpy(result, input.c_str());
        return result;
    }

    int Utils::writeBytesToFile(const std::string &filePath, const char *data, size_t size) {
        FILE *file = fopen(filePath.c_str(), "wb");
        if (file == nullptr) {
            return -1;
        }
        size_t wrote = fwrite(data, sizeof(char), size, file);
        fclose(file);
        return wrote;
    }

    std::string Utils::getFilePathWithoutExtension(const std::string &filePath) {
        size_t pos = filePath.find_last_of('.');
        if (pos != std::string::npos) {
            return filePath.substr(0, pos);
        }
        return std::string(filePath);
    }

}