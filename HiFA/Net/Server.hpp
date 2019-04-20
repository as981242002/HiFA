#ifndef NET_SERVER_HPP
#define NET_SERVER_HPP


#include<memory>
#include"EventLoop.hpp"
#include"EventLoopThreadPool.hpp"
#include"Channel.hpp"

class Server
{
public:
    Server(EventLoop* loop, int poolsize, int port);
    ~Server() = default;
    EventLoop* getLoop() const
    {
        return loop_;
    }
    void start();
    void handNewConn();
    void handThisConn()
    {
        loop_->updatePoller(acceptChannel_);
    }
private:
    EventLoop* loop_;
    int threadsize_;
    std::unique_ptr<EventLoopThreadPool> pool_;
    bool started_;
    std::shared_ptr<Channel> acceptChannel_;
    int port_;
    int listenFd_;
    static const int MAX_FD_SIZE = 100000;
};

#endif // SERVER_HPP
