#include<iostream>
#include<memory>
#include<stdint.h>
#include<errno.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/prctl.h>
#include<sys/types.h>
#include<linux/unistd.h>
#include "Thread.hpp"
#include "CountDownLatch.hpp"

using namespace std;

namespace CurrentThread
{
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char* t_threadName = "DefaultName";
}

pid_t gettid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}
