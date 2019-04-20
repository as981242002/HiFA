#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../Base/Logging.hpp"
#include "Util.hpp"
#include "Server.hpp"


Server::Server(EventLoop *loop, int poolsize, int port):
    loop_(loop),
    threadsize_(poolsize),
    port_(port),
    pool_(new EventLoopThreadPool(loop_, poolsize)),
    started_(false),
    acceptChannel_(new Channel(loop_)),
    listenFd_(socketBindListen(port_))
{
    acceptChannel_->setFd(listenFd_);
    handleForSigpipe();
    if(setSocketNonBlocking(listenFd_) < 0)
    {
        perror("set socket non block failed");
        abort();
    }

}

void Server::start()
{
    pool_->start();

    acceptChannel_->setEvents(EPOLLIN | EPOLLET);
    acceptChannel_->setReadHandler(bind(&Server::handNewConn, this));
    acceptChannel_->setConnHandler(bind(&Server::handThisConn, this));

    loop_->addToPoller(acceptChannel_, 0);
    started_  = true;
}

void Server::handNewConn()
{
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof (struct sockaddr_in));

    socklen_t client_addr_len = sizeof(client_addr);
    int accept_fd = 0;
    while ((accept_fd = accept(listenFd_, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len))> 0)
    {
        EventLoop* loop = pool_->getNextLoop();
        LOG << "new connection from " << inet_ntoa(client_addr.sin_addr)<< ":" <<  ntohs(client_addr.sin_port );

        if(accept_fd >= MAX_FD_SIZE)
        {
            close(accept_fd);
            continue;
        }
        if(setSocketNonBlocking(accept_fd) < 0)
        {
            LOG << "Set non block failed";
            return;
        }

        setSocketNodelay(accept_fd);

        shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd));
        req_info->getChannel()->setHolder(req_info);
        loop->queueInLoop(std::bind(&HttpData :: newEvent, req_info));
    }

    acceptChannel_ ->setEvents(EPOLLIN | EPOLLET);
}
