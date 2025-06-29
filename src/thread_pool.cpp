#include "thread_pool.hpp"

// コンストラクタ
ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        // ワーカースレッドを生成し、タスクを実行するラムダ関数を渡す
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    // タスクが空で、プールが停止していない限り待機
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });
                    // プールが停止し、かつタスクが空の場合はループを抜ける（スレッド終了）
                    if (this->stop && this->tasks.empty())
                        return;
                    // キューからタスクを取り出す
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                // タスクを実行
                task();
            }
        });
    }
}

// デストラクタ
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true; // プールを停止するフラグを設定
    }
    condition.notify_all(); // 待機中の全てのスレッドに通知

    // 全てのワーカースレッドが完了するのを待機
    for (std::thread& worker : workers) {
        worker.join();
    }
}