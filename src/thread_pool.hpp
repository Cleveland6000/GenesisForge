#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>

class ThreadPool {
public:
    // コンストラクタ: 指定された数のワーカースレッドを起動します。
    explicit ThreadPool(size_t threads);

    // デストラクタ: 全ての未処理タスクが完了し、スレッドが安全に終了するまで待機します。
    ~ThreadPool();

    // タスクをキューに追加し、非同期で実行します。
    // 戻り値としてstd::futureを返し、タスクの結果を取得したり、完了を待機したりできます。
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    // ワーカースレッドを保持するベクター
    std::vector<std::thread> workers;
    // 実行されるタスクのキュー
    std::queue<std::function<void()>> tasks;

    // タスクキューへのアクセスを保護するためのミューテックス
    std::mutex queue_mutex;
    // スレッドがタスクを待機するための条件変数
    std::condition_variable condition;

    // スレッドプールを停止するかどうかを示すフラグ
    bool stop;
};

// テンプレート関数の実装はヘッダーファイル内に配置する必要があります
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    // パッケージ化されたタスクを作成
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // プールが停止している場合はキューに入れることはできません
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one(); // 待機中のワーカースレッドにタスクが追加されたことを通知
    return res;
}

#endif