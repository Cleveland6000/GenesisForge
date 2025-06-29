#include "chunk_mesh_generator.hpp"
#include <glm/glm.hpp>
#include <array>
#include <vector>

// 立方体の基本頂点データ
// 各面の「基本頂点」は、面の形を定義する最小限の4つの頂点と考える
const std::vector<Vertex> baseCubeVertices = {
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 0: Back-bottom-left (Z-)
    {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f}, // 1: Back-bottom-right (Z-)
    {1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f}, // 2: Back-top-right (Z-)
    {0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}, // 3: Back-top-left (Z-)
    {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f}, // 4: Front-top-right (Z+)
    {1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}, // 5: Front-bottom-right (Z+)
    {0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f}, // 6: Front-bottom-left (Z+)
    {0.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f}  // 7: Front-top-left (Z+)
};

// 各面ごとの基本頂点インデックス（baseCubeVerticesへの参照）
// 各面は4つのユニークな頂点インデックスから構成されます。
// ここでの順序は、面を**外側から見たときに**、
// CCW（反時計回り）になるように定義されています。
// 具体的には、各面の最初の頂点（[0]）を基準とし、
// それを起点に反時計回りに他の頂点を並べます。
const std::array<std::array<unsigned int, 4>, 6> cubeFaceBaseIndices = {
    // 0: Back face (Z-): 法線(0,0,-1)
    // 外側から見てCCW: 0, 3, 2, 1 (左下、左上、右上、右下)
    std::array<unsigned int, 4>{0, 3, 2, 1},

    // 1: Front face (Z+): 法線(0,0,1)
    // 外側から見てCCW: 6, 5, 4, 7 (左下、右下、右上、左上)
    std::array<unsigned int, 4>{6, 5, 4, 7},

    // 2: Left face (X-): 法線(-1,0,0)
    // 外側から見てCCW: 0, 6, 7, 3 (左下、左上、右上、右下)
    std::array<unsigned int, 4>{0, 6, 7, 3},

    // 3: Right face (X+): 法線(1,0,0)
    // 外側から見てCCW: 1, 2, 4, 5 (左下、左上、右上、右下)
    std::array<unsigned int, 4>{1, 2, 4, 5},

    // 4: Bottom face (Y-): 法線(0,-1,0)
    // 外側から見てCCW: 0, 1, 5, 6 (左下、左上、右上、右下)
    std::array<unsigned int, 4>{0, 1, 5, 6},

    // 5: Top face (Y+): 法線(0,1,0)
    // 外側から見てCCW: 3, 7, 4, 2 (左下、左上、右上、右下)
    std::array<unsigned int, 4>{3, 7, 4, 2}};

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
// ChunkクラスのgetIndexと内容は同じですが、境界チェックを行わないため、
// ループ内で頻繁に呼ばれる際にオーバーヘッドを削減できます。
// ただし、呼び出し元で境界チェックを保証する必要があります。
inline size_t getVoxelIndex(int x, int y, int z, int chunkSize) {
    return static_cast<size_t>(x + y * chunkSize + z * chunkSize * chunkSize);
}

// generateMesh のシグネチャ変更
ChunkMeshData ChunkMeshGenerator::generateMesh(const Chunk &currentChunk, 
                                                 const std::array<const Chunk*, 6>& adjacentChunks)
{
    ChunkMeshData meshData;
    int chunkSize = currentChunk.getSize();
    const std::vector<bool>& voxels = currentChunk.getVoxels(); // 現在のチャンクのボクセルデータを直接取得

    // 最適化: 事前にベクタの容量を予約
    meshData.vertices.reserve(chunkSize * chunkSize * chunkSize * 4 * 6);
    meshData.indices.reserve(chunkSize * chunkSize * chunkSize * 6 * 6);

    for (int z = 0; z < chunkSize; ++z) // Z, Y, X の順にループすることでキャッシュヒット率を向上させる可能性がある
    {
        for (int y = 0; y < chunkSize; ++y)
        {
            for (int x = 0; x < chunkSize; ++x)
            {
                // getVoxelの代わりに直接voxels配列にアクセス
                if (voxels[getVoxelIndex(x, y, z, chunkSize)])
                { // 現在のボクセルが存在する場合
                    size_t currentVertexCount = meshData.vertices.size();

                    // 6つの各面についてチェック
                    for (int i = 0; i < 6; ++i)
                    {
                        glm::ivec3 offset = neighborOffsets[i];
                        int neighborX = x + offset.x;
                        int neighborY = y + offset.y;
                        int neighborZ = z + offset.z;

                        bool renderFace = true;
                        
                        // 隣接ボクセルが現在のチャンク内にあるかチェック
                        if (neighborX >= 0 && neighborX < chunkSize &&
                            neighborY >= 0 && neighborY < chunkSize &&
                            neighborZ >= 0 && neighborZ < chunkSize)
                        {
                            // チャンク内の隣接ボクセルが不透明な場合、面は描画しない
                            if (voxels[getVoxelIndex(neighborX, neighborY, neighborZ, chunkSize)])
                            {
                                renderFace = false;
                            }
                        }
                        // 隣接ボクセルが現在のチャンクの境界外（隣接チャンク内）にある場合
                        else 
                        {
                            const Chunk* adjChunk = adjacentChunks[i];
                            if (adjChunk != nullptr)
                            {
                                // 隣接チャンクのローカル座標に変換
                                // 例: x=0のチャンクの-1方向の隣接は、x=chunkSize-1の隣接チャンクに相当
                                int adj_x = (x + offset.x + chunkSize) % chunkSize;
                                int adj_y = (y + offset.y + chunkSize) % chunkSize;
                                int adj_z = (z + offset.z + chunkSize) % chunkSize;

                                // 隣接チャンクのボクセルをチェック
                                if (adjChunk->getVoxel(adj_x, adj_y, adj_z)) 
                                {
                                    renderFace = false;
                                }
                            }
                        }

                        if (renderFace)
                        {
                            // この面を描画する必要がある場合
                            // 各面の4つの基本頂点を追加
                            for (unsigned int baseIdx : cubeFaceBaseIndices[i])
                            {
                                Vertex baseVertex = baseCubeVertices[baseIdx];
                                Vertex newVertex = baseVertex;
                                // ボクセル位置にオフセットを適用
                                newVertex.x += x;
                                newVertex.y += y;
                                newVertex.z += z;
                                meshData.vertices.push_back(newVertex);
                            }

                            // 2つの三角形を形成する6つのインデックスを追加 (CCW順)
                            // 追加された4つの頂点に対して、相対インデックスは 0, 1, 2, 3 となる
                            // 三角形1: V0-V1-V2
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 0); // V0
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 1); // V1
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 2); // V2

                            // 三角形2: V0-V2-V3
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 0); // V0
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 2); // V2
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 3); // V3

                            // currentVertexCount は次の面のために更新
                            currentVertexCount += 4;
                        }
                    }
                }
            }
        }
    }

    return meshData;
}