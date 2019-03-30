cmake_minimum_required(VERSION 2.8)

set(base_SRCS
    ../Base/AsyncLogging.cpp
    ../Base/CountDownLatch.cpp
    ../Base/FileUtil.cpp
    ../Base/LogFile.cpp
    ../Base/LogStream.cpp
    ../Base/Logging.cpp
    ../Base/Thread.cpp
)

add_library(HiFA_base STATIC ${base_SRCS})
target_link_libraries(HiFA_base pthread rt)

