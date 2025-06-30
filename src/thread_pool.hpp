#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <iostream> // デバッグ出力のために追加

class ThreadPool {
public:
    ThreadPool(size_t threads);
    
    // タスクをキューに追加し、future を返す
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;

    ~ThreadPool();

private:
    // ワーカースレッドのコレクション
    std::vector<std::thread> workers;
    // タスクのキュー
    std::queue<std::function<void()>> tasks;
    
    // キューへのアクセスを同期するためのミューテックス
    std::mutex queue_mutex;
    // 条件変数 (タスクがあるか、スレッドがアイドル状態かを示す)
    std::condition_variable condition;
    // スレッドプールがシャットダウン中かどうかを示すフラグ
    bool stop;
};

// コンストラクタ
inline ThreadPool::ThreadPool(size_t threads)
    : stop(false)
{
    if (threads == 0) {
        throw std::invalid_argument("ThreadPool must have at least one thread.");
    }
    std::cout << "ThreadPool: Initializing with " << threads << " threads." << std::endl; // 追加

    for(size_t i = 0; i < threads; ++i) {
        std::cout << "ThreadPool: Attempting to create worker thread " << i << std::endl; // 追加
        workers.emplace_back(
            [this, i] // i をキャプチャしてデバッグメッセージに使う
            {
                std::cout << "ThreadPool: Worker thread " << i << " started." << std::endl; // 追加
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();
                }
                std::cout << "ThreadPool: Worker thread " << i << " exiting." << std::endl; // 追加
            }
        );
        std::cout << "ThreadPool: Worker thread " << i << " successfully created." << std::endl; // 追加
    }
    std::cout << "ThreadPool: All worker threads initialized." << std::endl; // 追加
}

// タスクをキューに追加
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // プールが停止している場合はキューに追加しない
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

// デストラクタ
inline ThreadPool::~ThreadPool()
{
    std::cout << "ThreadPool: Destructor called. Shutting down." << std::endl; // 追加
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers) {
        if (worker.joinable()) { // joinable かどうかチェック
            std::cout << "ThreadPool: Joining worker thread." << std::endl; // 追加
            worker.join();
        }
    }
    std::cout << "ThreadPool: All worker threads joined. Destructor finished." << std::endl; // 追加
}

#endif