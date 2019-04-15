#ifndef NET_EPOLL_HPP
#define NET_EPOLL_HPP

#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <memory>
#include "Timer.hpp"
#include "Channel.hpp"
#include "HttpData.hpp"


class Epoll
{
public:
    Epoll();
    ~Epoll() = default;
    void epollAdd(SP_Channel request, int timeout);
    void epollMod(SP_Channel request, int timeout);
    void epollDel(SP_Channel request, int timeout);
    std::vector<std::shared_ptr<Channel>> poll();
    std::vector<std::shared_ptr<Channel>> getEventsRequest(int events_num);
    void addTimer(std::shared_ptr<Channel>request_data, int timeout);
    int getEpollFd()
    {
        return epollFd_ ;
    }
    void handleExpired();
private:
    static const int MAXFDS = 100000;
    int epollFd_;
    std::vector<epoll_event> events_;
    std::shared_ptr<Channel> fd2chan_[MAXFDS];
    std::shared_ptr<HttpData> fd2http_[MAXFDS];
    TimerManager timemanager_;
};

#endif // NET_EPOLL_HPP
