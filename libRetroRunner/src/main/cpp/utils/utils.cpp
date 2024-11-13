#include <iostream>
#include <fstream>
#include <unistd.h>

#include "utils.h"

namespace libRetroRunner {

    Utils::ReadResult Utils::readFileAsBytes(const std::string &filePath) {
        std::ifstream fileStream(filePath);
        fileStream.seekg(0, std::ios::end);
        size_t size = fileStream.tellg();
        char *bytes = new char[size];
        fileStream.seekg(0, std::ios::beg);
        fileStream.read(bytes, size);
        fileStream.close();

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

}