#ifndef BASE_THREAD_HPP
#define BASE_THREAD_HPP

#include<functional>
#include<memory>
#include<string>
#include<unistd.h>
#include<pthread.h>
#include<sys/syscall.h>
#include "CountDownLatch.hpp"
#include "NonCopyable.h"

class Thread: NonCopyable
{
public:
    using ThreadFunc = std::function<void()>;
    explicit Thread(const ThreadFunc&, const std::string& name = std::string());
    ~Thread();
    void start();
    int join();
    bool started() const
    {
        return started_;
    }

    pid_t tid()
    {
        return tid_;
    }

    const std::string name() const
    {
        return name_;
    }
private:
    void setDefaultName();
    bool started_;
    bool joined_;
    pthread_t pthreadId_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    CountDownLatch latch_;
};

#endif // THREAD_HPP
