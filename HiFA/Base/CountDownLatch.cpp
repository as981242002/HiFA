#include "CountDownLatch.hpp"

void CountDownLatch::wait()
{
    MutexLockGugrd lock(mutex_);
    if(count_ > 0)
        condition_.wait();
}


void CountDownLatch::countDown()
{
    MutexLockGugrd lock(mutex_);
    --count_;
    if(count_ == 0)
        condition_.notifyAll();
}
