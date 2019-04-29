cmake_minimum_required(VERSION 2.8)

set(net_SRCS
	../Net/Timer.cpp
	../Net/Channel.cpp
	../Net/Epoll.cpp
	../Net/EventLoop.cpp
	../Net/EventLoopThread.cpp
	../Net/EventLoopThreadPool.cpp
	../Net/HttpData.cpp
	../Net/Server.cpp
	../Net/ThreadPool.cpp
	../Net/Timer.cpp
	../Net/Util.cpp)

add_library(HiFA_net STATIC ${net_SRCS})
target_link_libraries(HiFA_net pthread rt)
