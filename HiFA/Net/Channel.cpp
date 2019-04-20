#include<unistd.h>
#include<queue>
#include<cstdlib>
#include<iostream>
#include"Channel.hpp"
#include"Util.hpp"
#include"Epoll.hpp"
#include"EventLoop.hpp"

using namespace std;


Channel::Channel(EventLoop *loop):loop_(loop), events_(0), lastevents_(0)
{

}

Channel::Channel(EventLoop *loop, int fd):loop_(loop), fd_(fd), events_(0), lastevents_(0)
{

}

void Channel::handleRead()
{
    if(readHandler_)
    {
        readHandler_();
    }
}

void Channel::handleWrite()
{
    if(writeHandler_)
    {
        writeHandler_();
    }
}

void Channel::handleConn()
{
    if(connHandler_)
    {
        connHandler_();
    }
}
