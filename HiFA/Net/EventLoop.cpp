#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <iostream>
#include "../Base/Logging.hpp"
#include "EventLoop.hpp"
#include "Util.hpp"

using namespace std;

__thread EventLoop* t_loopInThisThread = 0;

int createEventfd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG << "Failed in eventfd";
        abort();
    }

    return evtfd;
}

EventLoop::EventLoop():
    looping_ (false),
    poller_(new Epoll()),
    wakeupFd_(createEventfd()),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    pwakeupChannel_(new Channel(this, wakeupFd_))
{
    if(t_loopInThisThread)
    {

    }
    else
    {
        t_loopInThisThread = this;
    }

    //ET MODE
    pwakeupChannel_ ->setEvents(EPOLLIN | EPOLLET);
    pwakeupChannel_ ->setReadHandler(bind(&EventLoop::handleRead, this));
    pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this));
    poller_->epollAdd(pwakeupChannel_, 0);
}

EventLoop::~EventLoop()
{
    close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    assert(!looping_);
    assert(isInloopThread());
    looping_ = true;
    quit_ = false;
    std::vector<SP_Channel> ret;
    while(!quit_)
    {
        ret.clear();
        ret = poller_->poll();
        eventHandling_ = true;
        for(auto& it:ret)
        {
            it->handleEvents();
        }
        eventHandling_ = false;
        doPendingFunctors();
        poller_->handleExpired();
    }
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if(!isInloopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(EventLoop::Functor &&cb)
{
    if(isInloopThread())
        cb();
    else
        queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(EventLoop::Functor &&cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }

    if(!isInloopThread() || callingPendingFunctors_)
        wakeup();
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, (char*)(&one), sizeof (one));
    if(n != sizeof (one))
    {
         LOG << "EventLoop::wakeup() writes" << n << "bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = readn(wakeupFd_, &one, sizeof (one));
    if(n != sizeof (one))
    {
        LOG << "EventLoop::handleRead() reads" <<  n << "bytes instead of 8";
    }

    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); i++)
    {
        functors[i]();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::handleConn()
{
    updatePoller(pwakeupChannel_, 0);
}
