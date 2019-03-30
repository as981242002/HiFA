#ifndef BASE_ASYNCLOGGING_H
#define BASE _ASYNCLOGGING_H

#include<functional>
#include<string>
#include<vector>
#include "CountDownLatch.hpp"
#include "MutexLock.hpp"
#include "Condition.hpp"
#include "FileUtil.hpp"
#include "NonCopyable.h"
#include "Thread.hpp"
#include "LogStream.hpp"

class AsyncLogging:NonCopyable
{
public:
    AsyncLogging(const std::string basename, int flushInterval = 2);
    ~AsyncLogging()
    {
        if(running_)
            stop();
    }

    void append(const char* logline, int len);
    void start()
    {
        running_ = true;
        thread_.start();
        count_.wait();
    }
    void stop()
    {
        running_ = false;
        cond_.notify();
        thread_.join();
    }
private:
    void threadFunc();
    const int flushInterval_;
    bool running_;
    std::string basename_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    CountDownLatch count_;
    using  Buffer  = FixedBuffer <kLargeBuffer>;
    using  BufferPtr = std::shared_ptr<Buffer>;
    using  BufferPtrVector =  std::vector<BufferPtr>;
    BufferPtr currentBuff_;
    BufferPtr nextBuff_;
    BufferPtrVector buffers_;
};
#endif // BASE_ASYNCLOGGING_H
