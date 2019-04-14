#include <queue>
#include <unistd.h>
#include <sys/time.h>
#include "Timer.hpp"


TimerNode::TimerNode(std::shared_ptr<HttpData> requestData, int timeout):
    deleted_(false), SPHttpData(requestData)
{
    struct timeval now;
    gettimeofday(&now, nullptr);
    expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

TimerNode::~TimerNode()
{
    if(SPHttpData)
        SPHttpData->handleClose();
}

TimerNode::TimerNode(TimerNode &tn):
    SPHttpData(tn.SPHttpData)
{

}

void TimerNode::update(int timeout)
{
    struct timeval now;
    gettimeofday(&now, nullptr);
    expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool TimerNode::isValid()
{
    struct timeval now;
    gettimeofday(&now, nullptr);
    size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
    if(temp < expiredTime_)
        return true;
    else
    {
        this->setDeleted();
        return false;
    }
}

void TimerNode::clearReq()
{
    SPHttpData.reset();
    this->setDeleted();
}


void TimerManager::addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout)
{
    SPTimerNode newnode(new TimerNode(SPHttpData, timeout));
    timerNodeQueue.push(newnode);
    SPHttpData->linkTimer(newnode);
}

void TimerManager::hanldeExpiredEvent()
{
    while(!timerNodeQueue.empty())
    {
        SPTimerNode ptimer_now = timerNodeQueue.top();
        if(ptimer_now->isDeleted())
        {
            timerNodeQueue.pop();
        }
        else if(!ptimer_now->isValid())
        {
            timerNodeQueue.pop();
        }
        else
            break;
    }
}
