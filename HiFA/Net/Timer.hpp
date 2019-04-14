#ifndef NET_TIMER_HPP
#define NET_TIMER_HPP

#include <unistd.h>
#include <memory>
#include <queue>
#include <deque>
#include "../Base/NonCopyable.h"
#include "HttpData.hpp"

class HttpData;

class TimerNode
{
public:
    TimerNode(std::shared_ptr<HttpData>requestData,int timeout);
    ~TimerNode();
    TimerNode(TimerNode& tn);
    void update(int timeout);
    bool isValid();
    void clearReq();
    void setDeleted()
    {
        deleted_ = true;
    }

    bool isDeleted() const
    {
        return deleted_;
    }

    size_t getExpTime() const
    {
        return expiredTime_;
    }
private:
    bool deleted_;
    size_t expiredTime_;
    std::shared_ptr<HttpData> SPHttpData;
};

struct TimerCmp
{
    bool operator()(std::shared_ptr<TimerNode>& lhs, std::shared_ptr<TimerNode>& rhs) const
    {
        return lhs->getExpTime() > rhs->getExpTime();
    }
};

class TimerManager
{
public:
    TimerManager() = default;
    ~TimerManager() = default;
    void addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout);
    void hanldeExpiredEvent();

private:
    using SPTimerNode = std::shared_ptr<TimerNode>;

    std::priority_queue<SPTimerNode, std::queue<SPTimerNode>, TimerCmp> timerNodeQueue;
};

#endif // NET_TIMER_HPP
