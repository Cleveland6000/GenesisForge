#include "chunk.hpp"
#include <stdexcept>

Chunk::Chunk(int size) : m_size(size), m_isDirty(true)
{
    if (size <= 0)
    {
        throw std::invalid_argument("Chunk size must be positive.");
    }
    m_voxels.resize(m_size * m_size * m_size);
}

void Chunk::setVoxel(int x, int y, int z, bool value)
{
    m_voxels[getIndex(x, y, z)] = value;
    m_isDirty = true;
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

void Chunk::setVoxels(const std::vector<bool> &voxels)
{
    if (voxels.size() != static_cast<size_t>(m_size * m_size * m_size))
    {
        throw std::invalid_argument("Input voxel data size does not match chunk dimensions.");
    }
    m_voxels = voxels;
    m_isDirty = true;
}
