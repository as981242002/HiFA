#ifndef NET_EVENTLOOP_HPP
#define NET_EVENTLOOP_HPP

#include<vector>
#include<memory>
#include<functional>
#include<iostream>
#include "../Base/Thread.hpp"
#include "../Base/Logging.hpp"
#include "../Base/CurrentThread.hpp"
#include "Channel.hpp"
#include "Epoll.hpp"
#include "Util.hpp"

using namespace std;
class EventLoop
{
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();
    void loop();
    void quit();
    void runInLoop(Functor&& cb);
    void queueInLoop(Functor&& cb);
    bool isInloopThread() const
    {
        return  threadId_ == CurrentThread::tid();
    }

    void assertInLoopThread()
    {
        assert(isInloopThread());
    }

    void shutdown(shared_ptr<Channel> channel)
    {
        poller_->epollDel(channel);
    }

    void updatePoller(shared_ptr<Channel> channel, int timeout = 0)
    {
        poller_->epollMod(channel, timeout);
    }

    void addToPoller(shared_ptr<Channel> channel, int timeout = 0)
    {
        poller_->epollAdd(channel, timeout);
    }
private:
    bool looping_;
    shared_ptr<Epoll> poller_;
    int wakeupFd_;
    bool quit_;
    bool eventHandling_;
    mutable MutexLock mutex_;
    vector<Functor> pendingFunctors_;
    bool callingPendingFunctors_;
    const pid_t threadId_;
    shared_ptr<Channel> pwakeupChannel_;

    void wakeup();
    void handleRead();
    void doPendingFunctors();
    void handleConn();
};

#endif // NET_EVENTLOOP_HPP
