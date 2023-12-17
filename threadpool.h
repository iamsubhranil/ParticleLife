#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

struct Threadpool {
   private:
    void loop() {
        while (true) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lck(queue_mutex);
                jobs_available.wait(
                    lck, [this] { return !jobs.empty() || should_terminate; });
                if (should_terminate) return;
                job = jobs.front();
                jobs.pop();
            }
            job();
            {
                std::unique_lock<std::mutex> lck1(queue_mutex);
                threads_done.notify_all();
            }
        }
    }

    std::mutex queue_mutex;
    std::condition_variable jobs_available;
    std::condition_variable threads_done;
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> jobs;
    std::atomic<bool> should_terminate;

   public:
    Threadpool(int numThreads = std::thread::hardware_concurrency())
        : should_terminate(false) {
        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back(std::thread(&Threadpool::loop, this));
        }
    }

    template <typename F, typename... Args>
    void submit(F&& func, Args&&... args) {
        {
            std::unique_lock<std::mutex> lck(queue_mutex);
            jobs.push([func, args...] { func(args...); });
        }
        jobs_available.notify_one();
    }

    void stop() {
        {
            std::unique_lock<std::mutex> lck(queue_mutex);
            should_terminate = true;
        }
        jobs_available.notify_all();
        for (auto& t : threads) {
            t.join();
        }
        threads.clear();
    }

    void join() {
        std::unique_lock<std::mutex> lock{queue_mutex};
        threads_done.wait(lock, [&]() { return jobs.empty(); });
    }

    ~Threadpool() {
        join();
        stop();
    }
};
