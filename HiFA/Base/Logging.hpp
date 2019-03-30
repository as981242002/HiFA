#ifndef BASE_Logger_HPP
#define BASE_Logger_HPP

#include<pthread.h>
#include<string.h>
#include<string>
#include<stdio.h>
#include"LogStream.hpp"

class AsyncLogger;

class Logger
{
public:
    Logger(const char* fileName, int line);
    ~Logger();

    LogStream& stream()
    {
        return impl_.stream_;
    }

    static void setLogFileName(std::string fileName)
    {
        logFileName_ = fileName;
    }

    static std::string getLogFileName()
    {
        return logFileName_;
    }
private:
    class Impl
    {
    public:
        Impl(const char* fileName, int line);
        void formatTime();

        LogStream stream_;
        int line_;
        std::string basename_;
    };

    Impl impl_;
    static std::string logFileName_;
};

#define LOG Logger(__FILE__, __LINE__).stream()
#endif // Logger_HPP
