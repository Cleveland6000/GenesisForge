#ifndef PERLINNOISE2D_HPP
#define PERLINNOISE2D_HPP

#include <vector>
#include <random>
#include <cmath>
#include <numeric>
#include <algorithm>

class PerlinNoise2D
{
public:
    PerlinNoise2D(unsigned int seed);
    float noise(float x, float y) const;

private:
    std::vector<int> p;
    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x, float y) const;
};

#endif // PERLINNOISE2D_HPP
