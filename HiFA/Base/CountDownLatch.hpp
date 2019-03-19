#ifndef BASE_COUNTDOWNLATCH_HPP
#define BASE_COUNTDOWNLATCH_HPP

#include "NonCopyable.h"
#include "Condition.hpp"
#include "MutexLock.hpp"

class CountDownLatch :NonCopyable
{
public:
    explicit CountDownLatch(int32_t count):mutex_(),count_(count),condition_(mutex_)
    {

    }

    void wait();
    void countDown();
private:
    mutable MutexLock mutex_;
    Condition condition_;
    int32_t count_;
};

#endif // COUNTDOWNLATCH_HPP
