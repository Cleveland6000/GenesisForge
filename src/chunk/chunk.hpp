#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <vector>

class Chunk {
public:
    explicit Chunk(int size);
    bool getVoxel(int x, int y, int z) const;
    void setVoxel(int x, int y, int z, bool value);
    void setVoxels(const std::vector<bool>& voxels);

    const std::vector<bool>& getVoxels() const { return m_voxels; }
    int getSize() const { return m_size; }
    bool isDirty() const { return m_isDirty; }
    void setDirty(bool dirty) { m_isDirty = dirty; }

private:
    size_t getIndex(int x, int y, int z) const;
    std::vector<bool> m_voxels;
    int m_size;
    bool m_isDirty;
};

#endif
