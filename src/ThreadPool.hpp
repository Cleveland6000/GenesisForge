// ThreadPool.hpp
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
#include <iostream> // デバッグログ用

class ThreadPool {
public:
    // コンストラクタ: スレッドプールの初期化とワーカースレッドの起動
    explicit ThreadPool(size_t threads)
        : stop(false)
    {
        if (threads == 0) {
            std::cerr << "Warning: ThreadPool initialized with 0 threads. Setting to 1.\n" << std::flush;
            threads = 1; // 少なくとも1つのスレッドを確保
        }
        std::cout << "DEBUG THREADPOOL: ThreadPool constructor called with " << threads << " threads.\n" << std::flush;
        
        std::cout << "DEBUG THREADPOOL: Starting worker thread creation loop.\n" << std::flush;
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back(
                [this, i] { // スレッドごとのラムダ関数
                    std::cout << "DEBUG THREADPOOL: Worker thread " << i << " started. Entering loop.\n" << std::flush;
                    while (true) {
                        std::function<void()> task;
                        
                        // スコープを作り、unique_lockがタスク取得後に確実に解放されるようにする
                        { 
                            std::cout << "DEBUG THREADPOOL: Worker thread " << i << " trying to acquire queue_mutex for wait.\n" << std::flush;
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            std::cout << "DEBUG THREADPOOL: Worker thread " << i << " acquired queue_mutex. Waiting for condition.\n" << std::flush;
                            
                            // condition.wait は述語がtrueになるまで待機し、待機中はロックを解放する
                            this->condition.wait(lock, 
                                [this]{ 
                                    return this->stop || !this->tasks.empty(); 
                                });
                            
                            std::cout << "DEBUG THREADPOOL: Worker thread " << i << " condition met. Stop: " << this->stop << ", Tasks empty: " << this->tasks.empty() << ".\n" << std::flush;
                            
                            // プールが停止中で、かつタスクキューが空であればスレッドを終了
                            if (this->stop && this->tasks.empty()) {
                                std::cout << "DEBUG THREADPOOL: Worker thread " << i << " stopping gracefully.\n" << std::flush;
                                return; 
                            }
                            // タスクをキューから取り出す
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                            std::cout << "DEBUG THREADPOOL: Worker thread " << i << " picked up task. Remaining tasks: " << this->tasks.size() << ".\n" << std::flush;
                        } // unique_lock のスコープがここで閉じ、queue_mutex は確実に解放される

                        std::cout << "DEBUG THREADPOOL: Worker thread " << i << " released queue_mutex. Executing task.\n" << std::flush;
                        
                        // タスクの実行
                        try {
                            task();
                            std::cout << "DEBUG THREADPOOL: Worker thread " << i << " finished task.\n" << std::flush;
                        } catch (const std::exception& e) {
                            std::cerr << "ERROR THREADPOOL: Worker thread " << i << " caught exception: " << e.what() << "\n" << std::flush;
                        } catch (...) {
                            std::cerr << "ERROR THREADPOOL: Worker thread " << i << " caught unknown exception.\n" << std::flush;
                        }
                    }
                }
            );
        }
        std::cout << "DEBUG THREADPOOL: All worker threads created in constructor.\n" << std::flush;
    }

    // デストラクタ: 全てのワーカースレッドを結合（終了を待つ）
    ~ThreadPool() {
        std::cout << "DEBUG THREADPOOL: ThreadPool destructor called. Setting stop flag.\n" << std::flush;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true; // 停止フラグを設定
        }
        condition.notify_all(); // 全ての待機中のスレッドに通知して終了を促す
        std::cout << "DEBUG THREADPOOL: Notified all worker threads. Joining threads...\n" << std::flush;
        for (std::thread &worker : workers) {
            if (worker.joinable()) {
                worker.join(); // スレッドが終了するのを待つ (デストラクタが完了する前に)
            }
        }
        std::cout << "DEBUG THREADPOOL: All worker threads joined. ThreadPool destroyed.\n" << std::flush;
    }

    // タスクをキューに追加し、Futureを返す
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<std::invoke_result_t<F, Args...>> // C++17以降ではstd::invoke_result_tを使用
    {
        using return_type = std::invoke_result_t<F, Args...>;

        std::cout << "DEBUG THREADPOOL: enqueue - Creating packaged_task...\n" << std::flush;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::cout << "DEBUG THREADPOOL: enqueue - Packaged_task created.\n" << std::flush;
        
        std::future<return_type> res = task->get_future();

        std::cout << "DEBUG THREADPOOL: Enqueuing new task. Queue size before: " << tasks.size() << "\n" << std::flush;
        
        // 新しいデバッグログを追加
        std::cout << "DEBUG THREADPOOL: enqueue - About to lock queue_mutex.\n" << std::flush;

        {
            // タスクキューへのアクセスを保護するためにミューテックスをロック
            std::unique_lock<std::mutex> lock(queue_mutex); 

            std::cout << "DEBUG THREADPOOL: enqueue - Mutex locked for task queue.\n" << std::flush;

            // プールが停止中の場合はエラー
            if (stop) {
                std::cerr << "ERROR THREADPOOL: enqueue called on stopped ThreadPool.\n" << std::flush;
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            // タスクをキューに追加
            tasks.emplace([task](){ (*task)(); });
            std::cout << "DEBUG THREADPOOL: enqueue - Task emplaced into queue.\n" << std::flush;
        } // unique_lock のスコープが閉じ、ミューテックスが解放される
        std::cout << "DEBUG THREADPOOL: enqueue - Mutex unlocked for task queue.\n" << std::flush;

        condition.notify_one(); // 1つのワーカースレッドに新しいタスクが利用可能であることを通知
        std::cout << "DEBUG THREADPOOL: Task enqueued, notifying one worker. Queue size after: " << tasks.size() << "\n" << std::flush;
        return res;
    }

private:
    // ワーカースレッドのコンテナ
    std::vector<std::thread> workers;
    // タスクキュー
    std::queue<std::function<void()>> tasks;

    // 同期プリミティブ
    // queue_mutex はタスクキューへのアクセスを保護する
    std::mutex queue_mutex; 
    std::condition_variable condition; // タスクが利用可能になったことをワーカーに通知するための条件変数
    bool stop; // プールが停止中かどうかを示すフラグ
};

#endif // THREAD_POOL_HPP
