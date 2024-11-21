

#ifndef _UTILS_H
#define _UTILS_H

#include <unistd.h>
#include <string>

namespace libRetroRunner {

    class Utils {
    public:
        struct ReadResult {
            size_t size;
            char *data;
        };

        //需要加入zip解压方法
        //

        static ReadResult readFileAsBytes(const std::string &filePath);

        static ReadResult readFileAsBytes(const int fileDescriptor);

        static int writeBytesToFile(const std::string &filePath, const char *data, size_t size);

        static const char *cloneToCString(const std::string &input);

        static size_t getFileSize(FILE *file);
        static std::string getFilePathWithoutExtension(const std::string &filePath);
    };

}

#endif
