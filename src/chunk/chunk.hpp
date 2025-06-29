#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <vector>
#include <glm/glm.hpp> // glm::ivec3 のために追加

class Chunk {
public:
    // コンストラクタに座標パラメータを追加
    explicit Chunk(int size, const glm::ivec3& coord); 
    
    bool getVoxel(int x, int y, int z) const;
    void setVoxel(int x, int y, int z, bool value);
    void setVoxels(const std::vector<bool>& voxels);

    const std::vector<bool>& getVoxels() const { return m_voxels; }
    int getSize() const { return m_size; }
    bool isDirty() const { return m_isDirty; }
    void setDirty(bool dirty) { m_isDirty = dirty; }

    // 新しく追加するメソッド
    glm::ivec3 getCoord() const { return m_coord; }

private:
    size_t getIndex(int x, int y, int z) const;
    std::vector<bool> m_voxels;
    int m_size;
    bool m_isDirty;
    glm::ivec3 m_coord; // チャンクのワールド座標
};

#endif // CHUNK_HPP