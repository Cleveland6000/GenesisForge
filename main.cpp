#include <iostream>
#include <vector>
#include <array>
#include <cmath> // std::sqrt, std::fmax など

// --- 格子ボルツマン法 (D2Q9モデル) の定数 ---
// 速度ベクトル: x方向
const std::array<int, 9> c_x = {0, 1, 0, -1, 0, 1, -1, -1, 1};
// 速度ベクトル: y方向
const std::array<int, 9> c_y = {0, 0, 1, 0, -1, 1, 1, -1, -1};
// 重み係数
const std::array<double, 9> weights = {
    4.0 / 9.0, // c0
    1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0, // c1-c4
    1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0 // c5-c8
};

// --- LatticeBoltzmannSimulator クラス ---
class LatticeBoltzmannSimulator {
public:
    // コンストラクタ: シミュレーション領域の幅と高さを設定
    LatticeBoltzmannSimulator(int w, int h) : width(w), height(h) {
        // 分布関数 (f_i) を格納するメモリを確保
        // 各セルに9方向のf_iがあるので、width * height * 9 のサイズ
        f.resize(width * height * 9);
        f_temp.resize(width * height * 9); // 衝突後の一時的な格納用

        // 初期化: 全てのf_iを平衡分布関数で初期化する
        // 初期密度: 1.0 (流体全体に均一に存在)
        // 初期速度: 0.0 (流体は静止状態から開始)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                double density = 1.0;
                double vel_x = 0.0;
                double vel_y = 0.0;
                for (int i = 0; i < 9; ++i) {
                    f[getIndex(x, y, i)] = calculateEquilibriumDistribution(i, density, vel_x, vel_y);
                }
            }
        }
    }

    // シミュレーションを1ステップ進めるメイン関数
    void update() {
        // フェーズ1: 衝突ステップ (Collision Step)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // その格子点のマクロ量（密度と速度）を現在のf_iから計算
                double density = getDensity(x, y);
                double vel_x, vel_y;
                getVelocity(x, y, density, vel_x, vel_y);

                // 平衡分布関数を計算し、BGKモデルで衝突後のf_iを更新
                for (int i = 0; i < 9; ++i) {
                    double feq = calculateEquilibriumDistribution(i, density, vel_x, vel_y);
                    // 緩和時間 tau を用いたBGKモデル
                    // tauは0.5より大きく、通常1.0付近
                    // tau = 1.0 は最も単純なケース（完全緩和）
                    const double tau = 1.0; 
                    f_temp[getIndex(x, y, i)] = f[getIndex(x, y, i)] - (f[getIndex(x, y, i)] - feq) / tau;
                }
            }
        }

        // フェーズ2: 並進ステップ (Streaming Step)
        // f_tempの値を、それぞれの速度方向に次の格子点に移動させる
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                for (int i = 0; i < 9; ++i) {
                    // 次の格子点の座標を計算
                    int next_x = x + c_x[i];
                    int next_y = y + c_y[i];

                    // 周期境界条件を適用
                    // 領域の端を越えたら反対側から出てくる
                    if (next_x < 0) next_x += width;
                    if (next_x >= width) next_x -= width;
                    if (next_y < 0) next_y += height;
                    if (next_y >= height) next_y -= height;

                    // 衝突後の値を次の格子点の対応する方向に代入
                    f[getIndex(next_x, next_y, i)] = f_temp[getIndex(x, y, i)];
                }
            }
        }
    }

    // 指定された格子点の密度を計算して返す
    double getDensity(int x, int y) const {
        double density = 0.0;
        for (int i = 0; i < 9; ++i) {
            density += f[getIndex(x, y, i)];
        }
        return density;
    }

    // 指定された格子点の速度を計算して返す
    void getVelocity(int x, int y, double density, double& vel_x, double& vel_y) const {
        vel_x = 0.0;
        vel_y = 0.0;
        for (int i = 0; i < 9; ++i) {
            vel_x += f[getIndex(x, y, i)] * c_x[i];
            vel_y += f[getIndex(x, y, i)] * c_y[i];
        }
        // 密度が0でなければ正規化 (ゼロ除算回避)
        if (density != 0.0) {
            vel_x /= density;
            vel_y /= density;
        }
    }

    // 全ての格子点の密度フィールドを取得（可視化用など）
    std::vector<double> getDensityField() const {
        std::vector<double> density_field(width * height);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                density_field[y * width + x] = getDensity(x, y);
            }
        }
        return density_field;
    }

private:
    int width, height; // シミュレーション領域の幅と高さ
    std::vector<double> f;      // 現在の分布関数 (f_i)
    std::vector<double> f_temp; // 衝突ステップで一時的に更新された分布関数

    // 平衡分布関数を計算するヘルパー関数
    double calculateEquilibriumDistribution(int i, double density, double ux, double uy) const {
        // c_i と u の内積 (dot product)
        double c_dot_u = c_x[i] * ux + c_y[i] * uy;
        // u と u の内積 (速度の2乗)
        double u_dot_u = ux * ux + uy * uy;

        // D2Q9モデルの平衡分布関数の式
        return weights[i] * density * (1.0 + 3.0 * c_dot_u + 4.5 * c_dot_u * c_dot_u - 1.5 * u_dot_u);
    }

    // (x, y, i) のインデックスを1次元配列のインデックスに変換するヘルパー関数
    // i は方向インデックス (0-8)
    int getIndex(int x, int y, int i) const {
        return (y * width + x) * 9 + i;
    }
};

// --- main 関数 ---
int main() {
    // シミュレーションの設定
    const int sim_width = 50;   // シミュレーション領域の幅
    const int sim_height = 50;  // シミュレーション領域の高さ
    const int num_steps = 1000; // シミュレーションの総ステップ数

    // LatticeBoltzmannSimulator オブジェクトを作成
    LatticeBoltzmannSimulator simulator(sim_width, sim_height);

    // シミュレーションループ
    for (int step = 0; step < num_steps; ++step) {
        simulator.update(); // シミュレーションを1ステップ進める

        // 進行状況をコンソールに表示 (100ステップごと)
        if (step % 100 == 0) {
            std::cout << "Step: " << step << std::endl;

            // 例: 中央のセルの密度を表示して、シミュレーションが生きているか確認
            double center_density = simulator.getDensity(sim_width / 2, sim_height / 2);
            std::cout << "  Center Density: " << center_density << std::endl;
        }
    }

    // 最終的な密度フィールドを取得し、最大/最小密度を表示
    std::vector<double> final_density_field = simulator.getDensityField();
    double min_den = 9999.0;
    double max_den = -9999.0;
    for (double d : final_density_field) {
        min_den = std::fmin(min_den, d);
        max_den = std::fmax(max_den, d);
    }
    std::cout << "\nSimulation finished." << std::endl;
    std::cout << "Final Density Range: Min=" << min_den << ", Max=" << max_den << std::endl;

    return 0;
}