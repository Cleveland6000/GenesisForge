#include "./perlin_noise_2d.hpp"
#include <iostream>
#include <numeric>
#include <random>
#include <algorithm>
#include <cmath>

float PerlinNoise2D::fade(float t) const {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float PerlinNoise2D::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

// 最適化された grad 関数
float PerlinNoise2D::grad(int hash, float x, float y) const {
    // 従来の switch と同じ結果を、ビット演算でより効率的に計算
    // ハッシュの最下位2ビットで0-3の4つのベクトル方向を決定し、
    // 残りのビットで正負を決定します。
    switch (hash & 0xF) { // 0xF (15) を使用して、元の8つのケースをカバー
        case 0x0: case 0x8: return  x + y; // 0000, 1000
        case 0x1: case 0x9: return -x + y; // 0001, 1001
        case 0x2: case 0xA: return  x - y; // 0010, 1010
        case 0x3: case 0xB: return -x - y; // 0011, 1011
        case 0x4: case 0xC: return  y + x; // 0100, 1100 (y,xと同じ)
        case 0x5: case 0xD: return -y + x; // 0101, 1101 (-y,xと同じ)
        case 0x6: case 0xE: return  y - x; // 0110, 1110 (y,-xと同じ)
        case 0x7: case 0xF: return -y - x; // 0111, 1111 (-y,-xと同じ)
        default: return 0; // ここには到達しないはず
    }
}


PerlinNoise2D::PerlinNoise2D(unsigned int seed) {
    p.resize(512); // 256要素をコピーするため、512要素確保
    std::iota(p.begin(), p.begin() + 256, 0);
    std::default_random_engine engine(seed);
    std::shuffle(p.begin(), p.begin() + 256, engine);
    // 256要素を複製して、ルックアップを高速化
    for (int i = 0; i < 256; ++i) {
        p[i + 256] = p[i];
    }
}

float PerlinNoise2D::noise(float x, float y) const {
    // intにキャストする際にfloorを適用し、結果を255でマスクする
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;

    // 小数部分を計算
    x -= std::floor(x);
    y -= std::floor(y);

    // フェード関数を適用
    float u = fade(x);
    float v = fade(y);

    // グリッド座標のハッシュ計算
    // p配列のサイズは512なので、p[X] + Y の結果が255を超えても問題ない
    int A = p[X] + Y;
    int B = p[X + 1] + Y;

    // 線形補間
    return lerp(
        lerp(grad(p[A], x, y),           // (0,0) グリッドセルの勾配
             grad(p[B], x - 1, y),       // (1,0) グリッドセルの勾配
             u),
        lerp(grad(p[A + 1], x, y - 1),   // (0,1) グリッドセルの勾配
             grad(p[B + 1], x - 1, y - 1), // (1,1) グリッドセルの勾配
             u),
        v
    );
}