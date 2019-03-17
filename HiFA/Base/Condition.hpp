#ifndef BASE_CONDITION_HPP
#define BASE_CONDITION_HPP

#include<pthread.h>
#include<errno.h>
#include<cstdint>
#include<time.h>
#include"NonCopyable.h"
#include"MutexLock.hpp"

class Condition:NonCopyable
{
public:
    explicit Condition(MutexLock& mutex):mutex_(mutex)
    {
        pthread_cond_init(&cond, NULL);
    }

    ~Condition()
    {
        pthread_cond_destroy(&cond);
    }

    void wait()
    {
        pthread_cond_wait(&cond, mutex_.get());
    }

    void notify()
    {
        pthread_cond_signal(&cond);
    }

    void notifyAll()
    {
        pthread_cond_broadcast(&cond);
    }

    bool waitForSeconds(int seconds)
    {
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += static_cast<time_t>(seconds);
        return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex_.get(), &abstime);
    }
private:
    MutexLock& mutex_;
    pthread_cond_t cond;
};

#endif // CONDITION_HPP
