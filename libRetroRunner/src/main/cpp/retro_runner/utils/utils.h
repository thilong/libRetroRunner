

#ifndef _UTILS_H
#define _UTILS_H

#include <unistd.h>
#include <string>
#include <vector>

namespace libRetroRunner {

    class Utils {
    public:

        static std::vector<unsigned char> readFileAsBytes(const std::string &filePath);

        static std::vector<unsigned char> readFileAsBytes(const int fileDescriptor);

        static int writeBytesToFile(const std::string &filePath, const char *data, size_t size);

        static size_t getFileSize(FILE *file);

        static std::string getFilePathWithoutExtension(const std::string &filePath);
    };

}

#endif
