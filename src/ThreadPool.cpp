// ThreadPool.cpp
#include "ThreadPool.hpp"

// ThreadPool::ThreadPool の定義
ThreadPool::ThreadPool(size_t threadCount)
    : m_stop(false), m_threadsReadyCount(0) // std::atomic は0でデフォルト初期化されるが、明示的に
{
    std::cout << "[ThreadPool] Starting " << threadCount << " threads\n";
    for (size_t i = 0; i < threadCount; ++i)
    {
        m_workers.emplace_back([this, i]() {
            std::cout << "[ThreadPool] Worker " << i << " started\n";

            // このワーカースレッドがタスクを受け取る準備ができたことを通知
            // m_threadsReadyCount は atomic なので、ロックは不要
            m_threadsReadyCount.fetch_add(1); // アトミックにインクリメント
            m_conditionReady.notify_one(); // コンストラクタに通知

            // タスクを処理するためのメインループ
            while (true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(m_queueMutex);
                    // タスクまたは停止シグナルを待機
                    m_condition.wait(lock, [this]() { return m_stop || !m_tasks.empty(); });

                    // 停止シグナルを受け取り、これ以上タスクがない場合はループとスレッドを終了
                    if (m_stop && m_tasks.empty())
                    {
                        return; // ワーカースレッドを終了
                    }

                    // 次のタスクを取得
                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                } // ロックはここで解放される

                // ロックの外でタスクを実行
                task();
            }
        });
    }

    // 明示的なバリア: すべてのスレッドが準備完了（m_threadsReadyCount が threadCount に到達）するまで待機
    // m_conditionReady.wait は依然として mutex が必要
    { // 新しいスコープを追加してロックの寿命を制限
        std::unique_lock<std::mutex> lock(m_queueMutex); // m_conditionReady.wait のためにロックを取得
        m_conditionReady.wait(lock, [this, threadCount]() { return m_threadsReadyCount.load() == threadCount; });
    } // ここでロックが解放される

    std::cerr << "gg\n";
}

// ThreadPool::~ThreadPool の定義
ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }
    m_condition.notify_all();
    for (std::thread &worker : m_workers)
    {
        if (worker.joinable())
            worker.join();
    }
}
