#ifndef NET_CHANNEL_HPP
#define NET_CHANNEL_HPP

#include <string>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>
#include <functional>
#include "Timer.hpp"

class EventLoop;
class HttpData;

class Channel
{
private:
    using CallBack = std::function<void()>;
    EventLoop* loop_;
    int fd_;
    uint32_t events_;
    uint32_t revents_;
    uint32_t lastevents_;

    std::weak_ptr<HttpData> holder_;
private:

    CallBack readHandler_;
    CallBack writeHandler_;
    CallBack errorHandler_;
    CallBack connHandler_;

public:
    Channel(EventLoop* loop);
    Channel(EventLoop* loop, int fd);
    ~Channel() = default;
    inline int getFd() const
    {
        return fd_;
    }
    inline void setFd(int fd)
    {
        fd_ = fd;
    }

    void setHolder(std::shared_ptr<HttpData> holder)
    {
        holder_ = holder;
    }

    std::shared_ptr<HttpData> getHolder()
    {
        return  holder_.lock();
    }

    void setReadHandler(CallBack&& readHandler)
    {
        readHandler_ = readHandler;
    }

    void setWriteHandler(CallBack&& writeHandler)
    {
        writeHandler_ = writeHandler;
    }

    void setErrorHandler(CallBack&& errorHandler)
    {
        errorHandler_ = errorHandler;
    }

    void setConnHandler(CallBack&& connHandler)
    {
        connHandler = connHandler_;
    }

    void handleEvents()
    {
        events_ = 0;
        if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
        {
            events_ = 0;
            return;
        }
        if(revents_ & EPOLLERR)
        {
            if(errorHandler_)
                errorHandler_();
            events_ = 0;
            return;
        }
        if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLHUP))
        {
            handleRead();
        }

        if(revents_ & EPOLLOUT)
        {
            handleWrite();
        }

    }

    void handleRead();
    void handleWrite();
    void handleConn();

    void setRevents(uint32_t ev)
    {
        revents_ = ev;
    }

    void setEvents(uint32_t ev)
    {
        events_ = ev;
    }

    uint32_t& getEvents()
    {
        return events_;
    }

    bool EqualAndUpdateLastEvents()
    {
        bool ret = (lastevents_ == events_);
        lastevents_ = events_;
        return ret;
    }

    uint32_t getLastEvents()
    {
        return lastevents_ ;
    }
};

using SP_Channel = std::shared_ptr<Channel>;
#endif // !CHANNEL_HPP
