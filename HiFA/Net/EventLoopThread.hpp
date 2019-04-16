#ifndef NET_EVENTLOOPTHREAD_HPP
#define NET_EVENTLOOPTHREAD_HPP

#include "../Base/Condition.hpp"
#include "../Base/Thread.hpp"
#include "../Base/MutexLock.hpp"
#include "../Base/NonCopyable.h"
#include "EventLoop.hpp"

class EventLoopThread:NonCopyable
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();
    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
};


#endif // NET_EVENTLOOPTHREAD_HPP
