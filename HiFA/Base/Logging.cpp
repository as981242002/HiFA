#include <assert.h>
#include <iostream>
#include <time.h>
#include <sys/time.h>
#include "Logging.hpp"
#include "CurrentThread.hpp"
#include "Thread.hpp"
#include "AsyncLogging.h"

static pthread_once_t once_flag= PTHREAD_ONCE_INIT;
static AsyncLogging* AsyncLogger_;

std::string Logger::logFileName_ = "/HiFA.log";

void once_init()
{
    AsyncLogger_ = new AsyncLogging(Logging::getLogFileName());
    AsyncLogger_->start();
}

void output(const char* msg, int len)
{
    pthread_once(&once_flag, once_init);
    AsyncLogger_->append(msg, len);
}


Logger::Impl::Impl(const char *fileName, int line):
    stream_(),
    line_(line),
    basename_(fileName)
{
    formatTime();
}

void Logger::Impl::formatTime()
{
    struct timeval tv;
    time_t time;
    char str_t[26] = {0};
    gettimeofday(&tv, nullptr);

}

Logger::Logger(const char* fileName, int line):
    impl_(fileName, line)
{

}

Logger::~Logger()
{
    impl_.stream_ << "--" << impl_.basename_ << ":" << impl_.line_ << "\n";
    const LogStream::Buffer& buf(stream().buffer());
    output(buf.data(), buf.length());
}
