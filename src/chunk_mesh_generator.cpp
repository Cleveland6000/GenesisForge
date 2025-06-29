#include "chunk_mesh_generator.hpp"
#include <glm/glm.hpp>
#include <array>
#include <vector>

// 立方体の基本頂点データ
// ここでは位置と色のみを定義し、UV座標は各面ごとに割り当てる
const std::vector<Vertex> baseCubeVertices = {
    // x, y, z, r, g, b, u, v (u,vはここでは0で仮置き。後で上書きされる)
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 0: Back-bottom-left (Z-)
    {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 1: Back-bottom-right (Z-)
    {1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f}, // 2: Back-top-right (Z-)
    {0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f}, // 3: Back-top-left (Z-)
    {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f}, // 4: Front-top-right (Z+)
    {1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}, // 5: Front-bottom-right (Z+)
    {0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f}, // 6: Front-bottom-left (Z+)
    {0.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f}  // 7: Front-top-left (Z+)
};

// 各面ごとの基本頂点インデックス（baseCubeVerticesへの参照）
// このインデックスは変更なし
const std::array<std::array<unsigned int, 4>, 6> cubeFaceBaseIndices = {
    // 0: Back face (Z-): 法線(0,0,-1)
    std::array<unsigned int, 4>{0, 3, 2, 1},
    // 1: Front face (Z+): 法線(0,0,1)
    std::array<unsigned int, 4>{6, 5, 4, 7},
    // 2: Left face (X-): 法線(-1,0,0)
    std::array<unsigned int, 4>{0, 6, 7, 3},
    // 3: Right face (X+): 法線(1,0,0)
    std::array<unsigned int, 4>{1, 2, 4, 5},
    // 4: Bottom face (Y-): 法線(0,-1,0)
    std::array<unsigned int, 4>{0, 1, 5, 6},
    // 5: Top face (Y+): 法線(0,1,0)
    std::array<unsigned int, 4>{3, 7, 4, 2}
};

// 各面に対応する隣接ボクセルのオフセット (x, y, z)
const std::array<glm::ivec3, 6> neighborOffsets = {
    glm::ivec3(0, 0, -1), // Back face (Z-)
    glm::ivec3(0, 0, 1),  // Front face (Z+)
    glm::ivec3(-1, 0, 0), // Left face (X-)
    glm::ivec3(1, 0, 0),  // Right face (X+)
    glm::ivec3(0, -1, 0), // Bottom face (Y-)
    glm::ivec3(0, 1, 0)   // Top face (Y+)
};

// ボクセル座標からm_voxelsのインデックスを計算するヘルパー関数
inline size_t getVoxelIndex(int x, int y, int z, int chunkSize) {
    return static_cast<size_t>(x + y * chunkSize + z * chunkSize * chunkSize);
}

ChunkMeshData ChunkMeshGenerator::generateMesh(const Chunk &chunk)
{
    ChunkMeshData meshData;
    int chunkSize = chunk.getSize();
    const std::vector<bool>& voxels = chunk.getVoxels();

    meshData.vertices.reserve(chunkSize * chunkSize * chunkSize * 4 * 6);
    meshData.indices.reserve(chunkSize * chunkSize * chunkSize * 6 * 6);

    // 各面のUV座標配列を定義
    // ここでは、テクスチャが反転している可能性を考慮してUV座標を定義しています。
    // もしテクスチャが上下逆さまに表示される場合は、faceUVsのv座標を反転させてみてください (例: 1.0f - v)。
    const std::array<glm::vec2, 4> faceUVs = {
        glm::vec2(0.0f, 0.0f), // 左下
        glm::vec2(1.0f, 0.0f), // 右下
        glm::vec2(1.0f, 1.0f), // 右上
        glm::vec2(0.0f, 1.0f)  // 左上
    };

    for (int z = 0; z < chunkSize; ++z)
    {
        for (int y = 0; y < chunkSize; ++y)
        {
            for (int x = 0; x < chunkSize; ++x)
            {
                if (voxels[getVoxelIndex(x, y, z, chunkSize)])
                {
                    size_t currentVertexCount = meshData.vertices.size();

                    for (int i = 0; i < 6; ++i)
                    {
                        glm::ivec3 offset = neighborOffsets[i];
                        int neighborX = x + offset.x;
                        int neighborY = y + offset.y;
                        int neighborZ = z + offset.z;

                        bool renderFace = true;
                        if (neighborX >= 0 && neighborX < chunkSize &&
                            neighborY >= 0 && neighborY < chunkSize &&
                            neighborZ >= 0 && neighborZ < chunkSize)
                        {
                            if (voxels[getVoxelIndex(neighborX, neighborY, neighborZ, chunkSize)])
                            {
                                renderFace = false;
                            }
                        }

                        if (renderFace)
                        {
                            // この面を描画する必要がある場合
                            for (int v_idx = 0; v_idx < 4; ++v_idx) // 4つの頂点についてループ
                            {
                                unsigned int baseIdx = cubeFaceBaseIndices[i][v_idx]; // 面のbaseCubeVerticesに対するインデックス
                                Vertex baseVertex = baseCubeVertices[baseIdx]; // 基本頂点から位置と色を取得

                                Vertex newVertex = baseVertex;
                                // ボクセル位置にオフセットを適用
                                newVertex.x += x;
                                newVertex.y += y;
                                newVertex.z += z;

                                // ここでテクスチャ座標を設定！
                                // cubeFaceBaseIndicesの順序が faceUVs の順序と対応するようにする
                                // 例えば、cubeFaceBaseIndices[i][0] が faceUVs[0] に、
                                // cubeFaceBaseIndices[i][1] が faceUVs[1] に対応する場合
                                newVertex.u = faceUVs[v_idx].x;
                                newVertex.v = faceUVs[v_idx].y;

                                meshData.vertices.push_back(newVertex);
                            }

                            // 2つの三角形を形成する6つのインデックスを追加 (CCW順)
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 0); // V0
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 1); // V1
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 2); // V2

                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 0); // V0
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 2); // V2
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 3); // V3

                            currentVertexCount += 4;
                        }
                    }
                }
            }
        }
    }

    return meshData;
}