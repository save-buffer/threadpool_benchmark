#include <condition_variable>
#include <thread>
#include <mutex>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <type_traits>

#include <omp.h>
#include <dispatch/dispatch.h>

#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

thread_local uint64_t ThreadLocalCounter = 0;

class ThreadPool
{
public:
    using Task = std::function<void(void)>;

    void Init();
    void Submit(Task fn);
    void AwaitEmpty();
    void Destroy();

private:
    static void WaitForWork(ThreadPool *pool);

    std::mutex mutex;
    std::condition_variable cv_task;
    std::condition_variable cv_idle;
    std::condition_variable cv_destroy;
    std::deque<Task> queue;
    int num_tasks;
    bool done;
    int num_finished;
    const uint32_t num_threads = std::thread::hardware_concurrency();
};

void ThreadPool::WaitForWork(ThreadPool *pool)
{
    std::unique_lock lk(pool->mutex);
    while(!pool->done)
    {
        while(!pool->queue.empty())
        {
            Task t = std::move(pool->queue.front());
            pool->queue.pop_front();
            lk.unlock();
            t();
            lk.lock();
            if(--pool->num_tasks == 0)
                pool->cv_idle.notify_all();
        }
        pool->cv_task.wait(lk);
    }
    pool->num_finished++;
    pool->cv_destroy.notify_one();
}

void ThreadPool::Init()
{
    num_tasks = 0;
    done = false;
    for(uint32_t i = 0; i < num_threads; i++)
    {
        std::thread t(ThreadPool::WaitForWork, this);
        t.detach();
    }
}

void ThreadPool::Submit(ThreadPool::Task t)
{
    {
        std::lock_guard guard(mutex);
        queue.emplace_back(std::move(t));
        num_tasks++;
    }
    cv_task.notify_one();
}

void ThreadPool::AwaitEmpty()
{
    std::unique_lock lk(mutex);
    cv_idle.wait(lk, [&]() { return num_tasks == 0; });
}

void ThreadPool::Destroy()
{
    std::unique_lock lk(mutex);
    done = true;
    num_finished = 0;
    cv_task.notify_all();
    cv_destroy.wait(lk, [&]() { return num_finished == num_threads; });
}

class OmpThreadPool
{
};

class GrandCentralThreadPool
{
public:
//    using Task = std::function<void(void)>;
    using Task = std::add_pointer_t<void(void)>;

    void Init()
    {
        queue = dispatch_queue_create_with_target(nullptr, DISPATCH_QUEUE_CONCURRENT, dispatch_get_global_queue(0, 0));
    }

    void Submit(Task _Nonnull t)
    {
//        __block Task tt = std::move(t);
//        dispatch_async(queue, ^() { tt(); });
        dispatch_async_f(queue, (void *)t, [](void * _Nonnull x) { ((Task)x)(); });
    }

    void AwaitEmpty()
    {
        dispatch_sync(queue, ^(){});
    }

    void Destroy()
    {
        dispatch_release(queue);
    }

private:
    dispatch_queue_t queue;
};

template <typename TP>
void BenchmarkStartup(ankerl::nanobench::Bench &bench, const char *name)
{
    bench.run(name,
              []()
              {
                  TP tp;
                  tp.Init();
                  tp.Destroy();
              });
}

template <>
void BenchmarkStartup<OmpThreadPool>(ankerl::nanobench::Bench &bench, const char *name)
{
    bench.run(name,
              []()
              {
                  omp_set_num_threads(std::thread::hardware_concurrency());
#pragma omp parallel
                  {
                      ThreadLocalCounter++;
                  }
                  omp_set_num_threads(1);
              });
}

template <typename TP>
void BenchmarkRunningTasks(ankerl::nanobench::Bench &bench, const char *name)
{
    constexpr int num_tasks = 1000 * 1000;
    TP tp;
    tp.Init();
    bench.batch(num_tasks);
    bench.warmup(3).minEpochIterations(10).run(name,
              [&]()
              {
                  for(int i = 0; i < num_tasks; i++)
                      tp.Submit([](){ ThreadLocalCounter++; });
                  tp.AwaitEmpty();
              });
    tp.Destroy();
}

template <>
void BenchmarkRunningTasks<OmpThreadPool>(ankerl::nanobench::Bench &bench, const char *name)
{
    constexpr int num_tasks = 1000 * 1000;
    bench.batch(num_tasks);
    {
        bench.warmup(3)
            .minEpochIterations(10)
            .run(name,
                 [&]()
                 {
                     #pragma omp parallel
                     #pragma omp single
                     for(int i = 0; i < num_tasks; i++)
                     {
                         #pragma omp task
                         { ThreadLocalCounter++; }
                     }
                 });
    }
}


int main()
{
    ankerl::nanobench::Bench startup;
    startup.title("StartupShutdown").unit("startup");
    ankerl::nanobench::Bench tasks;
    tasks.title("TaskingOverhead").unit("task");

#if 0
    BenchmarkStartup<ThreadPool>(startup, "Custom");
    BenchmarkStartup<OmpThreadPool>(startup, "OpenMP");
    BenchmarkStartup<GrandCentralThreadPool>(startup, "GCD");
#endif

    BenchmarkRunningTasks<GrandCentralThreadPool>(tasks, "GCD");
    BenchmarkRunningTasks<ThreadPool>(tasks, "Custom");
    BenchmarkRunningTasks<OmpThreadPool>(tasks, "OpenMP");
    return 0;
}
