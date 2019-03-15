#ifndef BASE_FILEUTIL_HPP
#define BASE_FILEUTIL_HPP

#include<string>
#include "NonCopyable.h"

class AppendFile : NonCopyable
{
public:
    explicit AppendFile(std::string filename);

    ~AppendFile();

    void append(const char* message, const size_t len);
    void flush();

private:
    size_t write(const char* message, const size_t len);
    FILE* fp_;
    constexpr static int buffer_size = 64;
    char buffer_[buffer_size * 1024];
};


#endif // BASE_FILEUTIL_HPP
