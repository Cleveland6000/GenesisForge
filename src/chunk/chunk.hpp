#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <vector>
#include <random>

class Chunk
{
public:
    Chunk(int size);
    bool getVoxel(int x, int y, int z) const;
    void setVoxel(int x, int y, int z, bool value);
    int getSize() const { return m_size; }

private:
    std::vector<bool> m_voxels;
    int m_size;
    size_t getIndex(int x, int y, int z) const;
};

#endif
