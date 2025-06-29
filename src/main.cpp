#include <iostream>
#include <thread> // std::thread::hardware_concurrency() を使用するため
#include <vector>
#include <future>   // std::future を使用するため
#include <chrono>   // std::chrono::milliseconds と std::this_thread::sleep_for を使用するため

// あなたの ThreadPool.hpp をインクルード
// ThreadPool.hpp と同じディレクトリに main.cpp があるか、
// コンパイラのインクルードパスに ThreadPool.hpp のディレクトリが含まれている必要があります。
#include "ThreadPool.hpp" 

// --- testThreadPool関数の定義 ---
// この関数は、以前に私が提供したテストコードです。
// 通常は別のファイル (例: test_utils.hpp / .cpp) に分けることもありますが、
// 今回は一時的なデバッグ目的のため、main.cpp に直接含めるか、
// main.cpp の前にインクルードする形で構いません。
void testThreadPool() {
    std::cout << "--- Starting ThreadPool Test ---\n" << std::flush;

    // 使用するスレッド数の決定
    // ハードウェアがサポートするスレッド数から1を引く（メインスレッド用）
    // 少なくとも1つのスレッドを確保
    size_t num_threads = std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() - 1 : 1;
    if (num_threads == 0) num_threads = 1;

    std::cout << "DEBUG MAIN: Initializing ThreadPool with " << num_threads << " threads.\n" << std::flush;
    // ThreadPoolオブジェクトの作成（コンストラクタがここで呼ばれる）
    ThreadPool pool(num_threads); 

    // スレッドプールが完全に初期化され、ワーカースレッドが待機状態になるのを待つための短い遅延
    // これは診断的な目的であり、根本的な問題の修正ではありませんが、
    // 稀なレースコンディションを緩和するのに役立つことがあります。
    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 2000ms待機 (2秒)

    std::cout << "DEBUG MAIN: ThreadPool initialized. Enqueuing tasks...\n" << std::flush;

    std::vector<std::future<int>> results; // タスクの結果を保持するfutureオブジェクトのベクター

    // 簡単なタスクをいくつかスレッドプールにエンキュー
    for (int i = 0; i < 5; ++i) {
        results.emplace_back(
            pool.enqueue([i]() { // ラムダ関数としてタスクを定義
                std::cout << "DEBUG TASK: Processing task " << i << " on thread " << std::this_thread::get_id() << "\n" << std::flush;
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // タスクの実行をシミュレートするため少し待機
                return i * i; // タスクの結果を返す
            })
        );
    }

    std::cout << "DEBUG MAIN: All tasks enqueued. Waiting for results...\n" << std::flush;

    // 各タスクの結果を取得
    // .get() はタスクが完了するまでブロックします
    for (int i = 0; i < results.size(); ++i) {
        try {
            int val = results[i].get(); // futureから結果を取得
            std::cout << "DEBUG MAIN: Task " << i << " completed with result: " << val << "\n" << std::flush;
        } catch (const std::exception& e) {
            std::cerr << "ERROR MAIN: Task " << i << " threw exception: " << e.what() << "\n" << std::flush;
        } catch (...) {
            std::cerr << "ERROR MAIN: Task " << i << " threw unknown exception.\n" << std::flush;
        }
    }

    std::cout << "DEBUG MAIN: All results retrieved.\n" << std::flush;
    std::cout << "--- ThreadPool Test Finished ---\n" << std::flush;
}

// --- main関数 ---
int main() {
    // まずはThreadPoolのテスト関数を呼び出す
    testThreadPool(); 

    // ここにあなたのゲームのメインループや初期化コードが続くでしょう
    // 例:
    // Application app;
    // app.run();

    std::cout << "Application finished.\n" << std::flush;
    return 0;
}