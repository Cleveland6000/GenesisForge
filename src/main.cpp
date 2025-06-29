#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <future> // std::future のために必要

#include "thread_pool.hpp" // 作成したスレッドプールヘッダーをインクルード

int main() {
    std::cout << "ThreadPool テストを開始します。" << std::endl;

    // ハードウェアがサポートするスレッド数を取得し、それに基づいてスレッドプールを初期化
    // 少なくとも2スレッド、または利用可能なスレッド数を使用
    size_t num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
        num_threads = 2; // ハードウェアスレッド数が不明な場合はデフォルトで2を使用
    }
    std::cout << "スレッドプールを " << num_threads << " スレッドで初期化します。" << std::endl;
    ThreadPool pool(num_threads);

    std::vector<std::future<int>> results;

    // 10個のタスクをスレッドプールに投入
    for (int i = 0; i < 10; ++i) {
        results.emplace_back(
            pool.enqueue([i] {
                std::cout << "タスク " << i << " がスレッド " << std::this_thread::get_id() << " で開始しました。" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100 + i * 50)); // 少し遅延させる
                std::cout << "タスク " << i << " がスレッド " << std::this_thread::get_id() << " で完了しました。" << std::endl;
                return i * i;
            })
        );
    }

    std::cout << "全てのタスクがスレッドプールにキューされました。" << std::endl;

    // 全てのタスクの結果を待機し、表示
    for (int i = 0; i < results.size(); ++i) {
        // .get() はタスクが完了するまでブロックします
        int value = results[i].get();
        std::cout << "タスク " << i << " の結果: " << value << std::endl;
    }

    std::cout << "ThreadPool テストが完了しました。" << std::endl;

    // ThreadPoolオブジェクトがスコープを抜けると、デストラクタが呼び出され、
    // 全てのスレッドが正常に終了するのを待機します。
    return 0;
}