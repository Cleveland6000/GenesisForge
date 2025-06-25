#include "Noise/PerlinNoise2D.hpp"
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

float PerlinNoise2D::grad(int hash, float x, float y) const {
    int h = hash & 7;
    switch (h) {
        case 0: { return x + y; }
        case 1: { return -x + y; }
        case 2: { return x - y; }
        case 3: { return -x - y; }
        case 4: { return y + x; }
        case 5: { return -y + x; }
        case 6: { return y - x; }
        case 7: { return -y - x; }
        default: { return 0; }
    }
}

PerlinNoise2D::PerlinNoise2D(unsigned int seed) {
    p.resize(512);
    std::iota(p.begin(), p.begin() + 256, 0);
    std::default_random_engine engine(seed);
    std::shuffle(p.begin(), p.begin() + 256, engine);
    for (int i = 0; i < 256; ++i) {
        p[i + 256] = p[i];
    }
}

float PerlinNoise2D::noise(float x, float y) const {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    x -= std::floor(x);
    y -= std::floor(y);
    float u = fade(x);
    float v = fade(y);
    int A = p[X] + Y;
    int B = p[X + 1] + Y;
    return lerp(
        lerp(grad(p[A], x, y), grad(p[B], x - 1, y), u),
        lerp(grad(p[A + 1], x, y - 1), grad(p[B + 1], x - 1, y - 1), u),
        v
    );
}
