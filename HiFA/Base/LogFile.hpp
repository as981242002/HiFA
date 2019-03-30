#ifndef BASE_LOGFILE_HPP
#define BASE_LOGFILE_HPP

#include<string>
#include<memory>
#include"FileUtil.hpp"
#include"MutexLock.hpp"
#include"NonCopyable.h"

class LogFile:NonCopyable
{
public:
    LogFile(const std::string& basename, int flushEveryN = 1024);
    ~LogFile() = default;

    void append(const char* logline, int len);
    void flush();
private:
    void appendUnlocked(const char* logline, int len);

    const std::string basename_;
    const int flushEveryN_; //append size

    int count_;
    std::unique_ptr<MutexLock> mutex_;
    std::unique_ptr<AppendFile> file_;
};

#endif // BASE_LOGFILE_HPP
