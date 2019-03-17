#ifndef BASE_MUTEXLOCK_HPP
#define BASE_MUTEXLOCK_HPP

#include<pthread.h>
#include<cstdio>
#include"NonCopyable.h"

class MutexLock:NonCopyable
{
public:
    MutexLock()
    {
        pthread_mutex_init(&mutex, NULL);
    }

    ~MutexLock()
    {
        pthread_mutex_lock(&mutex);
        pthread_mutex_destroy(&mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_t* get()
    {
        return &mutex;
    }
private:
    pthread_mutex_t mutex;
private:
    friend class Condition;
};

class MutexLockGugrd:NonCopyable
{
public:
    explicit MutexLockGugrd(MutexLock& mutex):mutex_(mutex)
    {
        mutex_.lock();
    }

    ~MutexLockGugrd()
    {
        mutex_.unlock();
    }
private:
    MutexLock& mutex_;
};

#endif // MUTEXLOCK_HPP
