// ThreadPool.hpp
#pragma once
#include <future>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>
#include <type_traits>
#include <iostream>
#include <atomic> // std::atomic を使用するために追加

class ThreadPool
{
public:
    // コンストラクタとデストラクタの宣言のみをここに残す
    ThreadPool(size_t threadCount);
    ~ThreadPool();

    template <typename Func, typename... Args>
    auto enqueue(Func &&f, Args &&...args)
        -> std::future<typename std::invoke_result_t<Func, Args...>>;

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;

    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    bool m_stop = false;

    // 新しいメンバー: スレッドの準備状況を同期するための条件変数とカウンター
    std::condition_variable m_conditionReady;
    std::atomic<size_t> m_threadsReadyCount; // std::atomic に変更
};

template <typename Func, typename... Args>
auto ThreadPool::enqueue(Func &&f, Args &&...args)
    -> std::future<typename std::invoke_result_t<Func, Args...>>
{
    using ReturnType = typename std::invoke_result_t<Func, Args...>;

    auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<Func>(f), std::forward<Args>(args)...));

    std::cerr << "EEE\n";
    std::future<ReturnType> res = taskPtr->get_future();

    std::cerr << "FFF\n";
    {
        std::cerr << "NN\n";
        std::unique_lock<std::mutex> lock(m_queueMutex);
        std::cerr << "YYY\n";
        if (m_stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");
        m_tasks.emplace([taskPtr]()
                        { (*taskPtr)(); });
    }
    std::cerr << "VVV\n";
    m_condition.notify_one();
    return res;
}