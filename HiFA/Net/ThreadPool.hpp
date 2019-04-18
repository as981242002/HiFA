#ifndef NET_THREADPOOL_HPP
#define NET_THREADPOOL_HPP

//TODO use C++17 ThreadPool
#include <pthread.h>
#include <functional>
#include <memory>
#include <vector>
#include "Channel.hpp"

enum class ThreadPoolStatus
{
    INVALID = -1,
    LOCK_FAILURE = -2,
    QUEUE_FULL = -3,
    SHUTDOWN = -4,
    THREAD_FAILURE = -5,
    NONE,
    GRACE_FULL = 1
};

enum class ShutDownOption
{
    IMMEDIDTE_SHUTDOWN = 1,
    GRACEFUL_SHUTDOWN = 2
};

struct ThreadPoolTask
{
    std::function<void(std::shared_ptr<void>)> func;
    std::shared_ptr<void> args;
};

const int MAX_THREAD_SIZE = 1024;
const int MAX_TASK_SIZE = 65535;

class ThreadPool
{
private:
    static pthread_mutex_t lock;
    static pthread_cond_t notify;

    static std::vector<pthread_t> works;
    static std::vector<ThreadPoolTask> tasks;
    static int worksize;
    static int tasksize;
    static int head;
    static int tailnext;
    static int count;
    static int shutdown;
    static int started;
public:
    static int Create(int, int);
    static int Add(std::shared_ptr<void> args, std::function<void(std::shared_ptr<void>)> func);
    static int Destroy(ShutDownOption option = ShutDownOption::GRACEFUL_SHUTDOWN);
    static int Free();
    static void* AllocTask(void* args);
};

#endif // NET_THREADPOOL_HPP
