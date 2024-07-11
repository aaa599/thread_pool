#include <iostream>
#include <type_traits>
#include <future>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <shared_mutex>

template <typename T>
class SafeQueue {
    std::queue<T> queue;
    std::mutex m_mutex;
public:
    bool empty() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return queue.empty();
    }
    size_t size() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return queue.size();
    }
    void push(T& item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        queue.push(item);
    }
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (queue.empty()) return false;
        item = std::move(queue.front());
        queue.pop();
        return true;
    }
};

class ThreadPool {
private:
    class Worker {
    public:
        ThreadPool* pool;
        explicit Worker(ThreadPool* pool) : pool(pool) {};
        void operator()() {
            while (!this->pool->shutdown) {
                {
                    std::unique_lock<std::mutex> lock(this->pool->queue_mutex);
                    this->pool->cond.wait(lock, [this](){
                        return !this->pool->work_queue.empty() || this->pool->shutdown;
                    });
                }
                std::function<void()> task;
                bool flag = this->pool->work_queue.pop(task);
                if (flag) {
                    task();
                }
            }
        }
    };
public:
    std::mutex queue_mutex;
    std::condition_variable cond;
    SafeQueue<std::function<void()>> work_queue;
    std::vector<std::thread> work_thread;
    bool shutdown;
    ThreadPool(int thread_num) : work_thread(thread_num), shutdown(false) {
        for (auto & thread : work_thread) {
            thread = std::thread{Worker(this)};
        }
    }
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    template<typename F, typename ...Args>
    auto submit(F&& f, Args&& ...args) ->std::future<decltype(f(args...))> {
        auto task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::function<void()> func = [task](){
            (*task)();
        };
        work_queue.push(func);
        cond.notify_one();
        return task->get_future();
    }
    ~ThreadPool() {
        auto f = submit([](){});
        f.get();
        shutdown = true;
        cond.notify_all();
        for (auto &t : work_thread) {
            if (t.joinable()) t.join();
        }
    }
};


std::mutex _m;
int main() {
    {
        ThreadPool threadPool(5);
        for (int i = 0; i < 20; ++i) {
            threadPool.submit([i](){
                std::unique_lock<std::mutex> lock(_m);
                std::cout << i << std::endl;
            });
        }
    }
//    {
//        ThreadPool pool(8);
//        int n = 20;
//        for (int i = 1; i <= n; i++) {
//            pool.submit([](int id) {
//                if (id % 2 == 1) {
//                    std::this_thread::sleep_for(std::chrono::seconds(1));
//                }
//                std::unique_lock<std::mutex> lc(_m);
//                std::cout << "id : " << id << std::endl;
//            }, i);
//        }
//    }
    return 0;
}
