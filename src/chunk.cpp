#include "chunk.hpp"
#include "Noise/PerlinNoise2D.hpp"
#include <stdexcept>
#include <random>
#include <chrono>

Chunk::Chunk(int size, float density) : m_size(size)
{
    if (size <= 0)
    {
        throw std::invalid_argument("Chunk size must be positive.");
    }
    if (density < 0.0f || density > 1.0f)
    {
        throw std::invalid_argument("Voxel density must be between 0.0 and 1.0.");
    }

    m_voxels.resize(m_size * m_size * m_size);

    unsigned int seed = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
    PerlinNoise2D perlin(seed);

    float scale = 0.05f;

    for (int x = 0; x < m_size; ++x)
    {
        for (int y = 0; y < m_size; ++y)
        {
            for (int z = 0; z < m_size; ++z)
            {

                m_voxels[getIndex(x, y, z)] = (perlin.noise(x * scale, z * scale) * 6) + 5 >= y;
            }
        }
    }
}

size_t Chunk::getIndex(int x, int y, int z) const
{
    if (x < 0 || x >= m_size || y < 0 || y >= m_size || z < 0 || z >= m_size)
    {
        throw std::out_of_range("Voxel coordinates out of chunk bounds.");
    }
    return static_cast<size_t>(x + y * m_size + z * m_size * m_size);
}

bool Chunk::getVoxel(int x, int y, int z) const
{
    return m_voxels[getIndex(x, y, z)];
}
