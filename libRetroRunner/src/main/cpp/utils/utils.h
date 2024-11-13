

#ifndef _UTILS_H
#define _UTILS_H
#include <unistd.h>
#include <string>

namespace libRetroRunner {

class Utils {
public:
    struct ReadResult {
        size_t size;
        char* data;
    };

    static ReadResult readFileAsBytes(const std::string &filePath);
    static ReadResult readFileAsBytes(const int fileDescriptor);

    static const char* cloneToCString(const std::string &input);

    static size_t getFileSize(FILE* file);
};

}

#endif
