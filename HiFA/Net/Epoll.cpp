#include <errno.h>
#include <string.h>
#include <assert.h>
#include <deque>
#include <queue>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include "Epoll.hpp"
#include "Util.hpp"
#include "../Base/Logging.hpp"

using namespace std;
const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000;

using SP_Channel = shared_ptr<Channel>;

Epoll::Epoll():
    epollFd_(epoll_create1(EPOLL_CLOEXEC)),
    events_(EVENTSNUM)
{
    assert(epollFd_ > 0);
}

void Epoll::epollAdd(SP_Channel request, int timeout)
{
    int fd = request->getFd();
    if(timeout > 0)
    {
        addTimer(request, timeout);
        fd2http_[fd] = request->getHolder();
    }
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();

    request->EqualAndUpdateLastEvents();

    fd2chan_[fd] = request;
    if(epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        perror("epoll add error");
        fd2chan_[fd].reset();
    }
}

void Epoll::epollMod(SP_Channel request, int timeout)
{
    if(timeout > 0)
    {
        addTimer(request, timeout);
    }
    int fd = request->getFd();
    if(!request->EqualAndUpdateLastEvents())
    {
        struct epoll_event event;
        event.data.fd = fd;
        event.events = request->getEvents();
        if(epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0)
        {
            perror("epoll mod error");
            fd2chan_[fd].reset();
        }
    }
}

void Epoll::epollDel(SP_Channel request, int timeout)
{
     int fd = request->getFd();
     struct  epoll_event event;
     event.data.fd = fd;
     event.events = request->getLastEvents();
     if(epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0)
     {
         perror("epoll del error");
     }
     fd2chan_[fd].reset();
     fd2http_[fd].reset();
}

std::vector<std::shared_ptr<Channel> > Epoll::poll()
{
    while(true)
    {
        int event_count = epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
        if(event_count < 0)
        {
            perror("epoll wait error");
        }
        std::vector<SP_Channel> req_data = getEventsRequest(event_count);
        if(req_data.size() > 0)
        {
            return req_data;
        }
    }
}

std::vector<std::shared_ptr<Channel> > Epoll::getEventsRequest(int events_num)
{
    std::vector<SP_Channel> req_data;
    for(int i = 0; i < events_num; ++i)
    {
        int fd = events_[i].data.fd;

        SP_Channel cur_req = fd2chan_[fd];

        if(cur_req)
        {
            cur_req->setRevents(events_[i].events);
            cur_req->setEvents(0);
            req_data.push_back(cur_req);
        }
        else
        {
            LOG << "SP cur_req is invalid";
        }
    }

    return req_data;
}

void Epoll::addTimer(std::shared_ptr<Channel> request_data, int timeout)
{
    shared_ptr<HttpData> t = request_data->getHolder();
    if(t)
        timemanager_.addTimer(t, timeout);
    else
        LOG << "timer add fail";
}

void Epoll::handleExpired()
{
    timemanager_.hanldeExpiredEvent();
}
