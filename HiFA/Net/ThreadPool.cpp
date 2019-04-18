#include "ThreadPool.hpp"

pthread_mutex_t  ThreadPool::lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::notify = PTHREAD_COND_INITIALIZER;
std::vector<pthread_t> ThreadPool::works;
std::vector<ThreadPoolTask> ThreadPool::tasks;
int ThreadPool::worksize = 0;
int ThreadPool::tasksize = 0;
int ThreadPool::head = 0;
int ThreadPool::tailnext = 0;
int ThreadPool::count = 0;
int ThreadPool::shutdown = 0;
int ThreadPool::started = 0;



int ThreadPool::Create(int threadcount, int taskcount)
{
    if(threadcount <=0 || threadcount > MAX_THREAD_SIZE || taskcount <= 0 ||  taskcount > MAX_TASK_SIZE)
    {
        threadcount = 4;
        taskcount = 1024;
    }

    worksize = 0;
    tasksize = taskcount;
    head = tailnext = count = 0;
    shutdown = started = 0;

    works.resize(threadcount);
    tasks.resize(taskcount);

    for(int i = 0; i < threadcount; ++i)
    {
        if(pthread_create(&works[i], nullptr, AllocTask, (void*) (0)) != 0)
        {
            return -1;
        }

        ++worksize;
        ++started;
    }
}

int ThreadPool::Add(std::shared_ptr<void> args, std::function<void (std::shared_ptr<void>)> func)
{
    int next, err =0;
    if(pthread_mutex_lock(&lock) != 0)
        return static_cast<int>(ThreadPoolStatus::LOCK_FAILURE);
    do
    {
        next = (tailnext + 1) % tasksize;

        if(count == tasksize)
        {
            err = static_cast<int>(ThreadPoolStatus::QUEUE_FULL);
            break;
        }

        if(shutdown)
        {
            err = static_cast<int>(ThreadPoolStatus::SHUTDOWN);
            break;
        }

        tasks[tailnext].func = func;
        tasks[tailnext].args = args;
        tailnext = next;
        ++count;

        if(pthread_cond_signal(&notify) != 0)
        {
            err = static_cast<int>(ThreadPoolStatus::LOCK_FAILURE);
            break;
        }

    }while(false);


    if(pthread_mutex_unlock(&lock) != 0)
    {
        err = static_cast<int>(ThreadPoolStatus::LOCK_FAILURE);
    }

    return err;

}

int ThreadPool::Destroy(ShutDownOption option)
{
    int i, err = 0;

    if(pthread_mutex_lock(&lock) != 0)
        return static_cast<int>(ThreadPoolStatus::LOCK_FAILURE);
    do
    {
        if(shutdown)
        {
            err = static_cast<int>(ThreadPoolStatus::SHUTDOWN);
            break;
        }

        shutdown = static_cast<int>(option);

        if((pthread_cond_broadcast(&notify) != 0) || pthread_mutex_unlock(&lock) != 0 )
        {
            err = static_cast<int>(ThreadPoolStatus::LOCK_FAILURE);
            break;
        }

        for(int i = 0; i < worksize; ++i)
        {
            if(pthread_join(works[i], nullptr) != 0)
            {
                err = static_cast<int>(ThreadPoolStatus::THREAD_FAILURE);
            }
        }
    } while (false);

    if(!err)
    {
        Free();
    }

    return  err;
}

int ThreadPool::Free()
{
    if(started > 0)
        return -1;
    pthread_mutex_lock(&lock);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&notify);

    return 0;
}

void* ThreadPool::AllocTask(void* args)
{
    while(true)
    {
        ThreadPoolTask task;
        pthread_mutex_lock(&lock);

        while((count == 0) && (!shutdown))
        {
            pthread_cond_wait(&notify, &lock);
        }

        if((shutdown == static_cast<int>(ShutDownOption::IMMEDIDTE_SHUTDOWN)) || ( (shutdown == static_cast<int>(ShutDownOption::GRACEFUL_SHUTDOWN)) && (count == 0)))
        {
            break;
        }

        task.func = tasks[head].func;
        task.args = tasks[head].args;
        tasks[head].func = nullptr;
        tasks[head].args.reset();

        head = (head+1) % tasksize;
        --count;
        pthread_mutex_unlock(&lock);
        (task.func)(task.args);
    }
    --started;
    pthread_mutex_unlock(&lock);
    pthread_exit(nullptr);

    return nullptr;
}
