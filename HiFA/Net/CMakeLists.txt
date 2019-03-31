cmake_minimum_required(VERSION 2.8)

set(net_SRCS
    ../Net/Timer.cpp)

add_library(HiFA_net STATIC ${net_SRCS})
target_link_libraries(HiFA_net pthread rt)
