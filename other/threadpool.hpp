#include <iostream>
#include <pthread.h>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread> 
using namespace std;
class Threadpool
{
    public:
    static mutex printMutex; 
    Threadpool(size_t p_num):stop(false),pendingTasks(0)
    {
        for(int i = 0;i < p_num;i++)
        {
            pthread_t thread;
            workers.emplace_back(&Threadpool::workerFunction, this);
        }
    }
    ~Threadpool()
    {
        {
        unique_lock<std::mutex> lock(queueMutex);
        stop = true;
        condition.notify_all();
        }
        for(std::thread &worker : workers)
        {
            worker.join();
            //pthread_join(worker,nullptr);
        }
    }

    template <class F>
    void enqueue(F&& f)
    {
        unique_lock<std::mutex> lock(queueMutex);
        if(stop)
        {
            throw std::runtime_error("ThreadPool has been stopped");
        }
        ++pendingTasks;
        taskQueue.emplace(forward<F>(f));
    
        condition.notify_one();
    }

    void wait()
{
    unique_lock lock(queueMutex);
    condition.wait(lock, [this] {
        return taskQueue.empty() && pendingTasks == 0;
    });
}

    private:
    void workerFunction()
    {
        //Threadpool* pool = static_cast<Threadpool*>(arg);
        while(true)
        {
            function<void()> task;
            {
             unique_lock<mutex> lock(queueMutex);
             condition.wait(lock, [this] {
                return stop || !taskQueue.empty();
                });
                
                if (stop && taskQueue.empty())
                return;
                
                task = move(taskQueue.front());
                taskQueue.pop();

            }
            task();
            --pendingTasks;
            if (pendingTasks == 0 && taskQueue.empty())
            {
                condition.notify_all();
            }
        }
        //return;

    }
    private:
vector<thread> workers; // 工作线程
queue<function<void()>> taskQueue; // 任务队列
mutex queueMutex; // 保护任务队列的互斥锁
condition_variable condition; // 条件变量，用于线程同步
atomic<bool> stop; // 标志，表示是否停止线程池
// static mutex printMutex; 
atomic<int> pendingTasks; // 记录未完成的任务数量

};
mutex Threadpool::printMutex;

