#ifndef NET_EVENTLOOPTHREADPOOL_HPP
#define MET_EVENTLOOPTHREADPOOL_HPP

#include <memory>
#include <vector>
#include "../Base/NonCopyable.h"
#include "../Base/Logging.hpp"
#include "EventLoopThread.hpp"

class EventLoopThreadPool:NonCopyable
{
public:
    EventLoopThreadPool(EventLoop* loop, int size);

    ~EventLoopThreadPool()
    {
        LOG << "Destroy EventLoopThreadPool";
    }

    void start();

    EventLoop* getNextLoop();

private:
    EventLoop* baseLoop_;
    bool started_;
    int size_;
    int next_;
    std::vector<std::shared_ptr<EventLoopThread>> workers_;
    std::vector<EventLoop*> loops_;
};

#endif // EVENTLOOPTHREADPOOL_HPP
