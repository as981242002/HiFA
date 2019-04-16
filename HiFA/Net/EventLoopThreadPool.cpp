#include "EventLoopThreadPool.hpp"


EventLoopThreadPool::EventLoopThreadPool(EventLoop* loop, int size):
    baseLoop_(loop),
    size_(size),
    started_(false),
    next_(0)
{
    if(size <= 0)
    {
        LOG << "ThreadPool size less or equal 0";
        exit(1);
    }
}

void EventLoopThreadPool::start()
{
    baseLoop_->assertInLoopThread();
    started_ = true;
    for(int i = 0; i < size_; ++i)
    {
        std::shared_ptr<EventLoopThread> t = make_shared<EventLoopThread>();
        workers_.push_back(t);
        loops_.push_back(t->startLoop());
    }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    assert(started_);
    EventLoop* loop = baseLoop_;
    if(!loops_.empty())
    {
        loop = loops_[next_];
        next_ = (next_ + 1) % size_;
    }

    return loop;
}
