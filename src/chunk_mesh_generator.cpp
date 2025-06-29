#include "chunk_mesh_generator.hpp"
#include <glm/glm.hpp>
#include <array>
#include <vector>

// 立方体の基本頂点データ
// Vertex構造体がx,y,z,r,g,b,u,v,nx,ny,nzを含むようになったため、初期化を修正
const std::vector<Vertex> baseCubeVertices = {
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 0: Back-bottom-left (Z-)
    {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 1: Back-bottom-right (Z-)
    {1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 2: Back-top-right (Z-)
    {0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 3: Back-top-left (Z-)
    {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 4: Front-top-right (Z+)
    {1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 5: Front-bottom-right (Z+)
    {0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // 6: Front-bottom-left (Z+)
    {0.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}  // 7: Front-top-left (Z+)
};

const std::array<std::array<unsigned int, 4>, 6> cubeFaceBaseIndices = {
    std::array<unsigned int, 4>{0, 3, 2, 1}, // 0: Back face (Z-)
    std::array<unsigned int, 4>{6, 5, 4, 7}, // 1: Front face (Z+)
    std::array<unsigned int, 4>{0, 6, 7, 3}, // 2: Left face (X-)
    std::array<unsigned int, 4>{1, 2, 4, 5}, // 3: Right face (X+)
    std::array<unsigned int, 4>{0, 1, 5, 6}, // 4: Bottom face (Y-)
    std::array<unsigned int, 4>{3, 7, 4, 2}  // 5: Top face (Y+)
};

// neighborOffsets は chunk_mesh_generator.hpp に移動済み

const std::array<glm::vec3, 6> faceNormals = {
    glm::vec3(0.0f, 0.0f, -1.0f), // 0: Back face (Z-)
    glm::vec3(0.0f, 0.0f, 1.0f),  // 1: Front face (Z+)
    glm::vec3(-1.0f, 0.0f, 0.0f), // 2: Left face (X-)
    glm::vec3(1.0f, 0.0f, 0.0f),  // 3: Right face (X+)
    glm::vec3(0.0f, -1.0f, 0.0f), // 4: Bottom face (Y-)
    glm::vec3(0.0f, 1.0f, 0.0f)   // 5: Top face (Y+)
};

const std::array<glm::vec2, 4> faceUVs = {
    glm::vec2(0.0f, 0.0f), // 左下
    glm::vec2(1.0f, 0.0f), // 右下
    glm::vec2(1.0f, 1.0f), // 右上
    glm::vec2(0.0f, 1.0f)  // 左上
};


inline size_t getVoxelIndex(int x, int y, int z, int chunkSize) {
    return static_cast<size_t>(x + y * chunkSize + z * chunkSize * chunkSize);
}

ChunkMeshData ChunkMeshGenerator::generateMesh(const Chunk &chunk,
                                              const Chunk* neighbor_neg_x,
                                              const Chunk* neighbor_pos_x,
                                              const Chunk* neighbor_neg_y,
                                              const Chunk* neighbor_pos_y,
                                              const Chunk* neighbor_neg_z,
                                              const Chunk* neighbor_pos_z)
{
    ChunkMeshData meshData;
    int chunkSize = chunk.getSize();
    const std::vector<bool>& voxels = chunk.getVoxels();

    meshData.vertices.reserve(chunkSize * chunkSize * chunkSize * 4 * 6);
    meshData.indices.reserve(chunkSize * chunkSize * chunkSize * 6 * 6);

    for (int z = 0; z < chunkSize; ++z)
    {
        for (int y = 0; y < chunkSize; ++y)
        {
            for (int x = 0; x < chunkSize; ++x)
            {
                if (voxels[getVoxelIndex(x, y, z, chunkSize)])
                {
                    size_t currentVertexCount = meshData.vertices.size();

                    for (int i = 0; i < 6; ++i) // 6面すべてについてループ
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
                            // 現在のチャンク内に隣接ボクセルがある場合
                            if (voxels[getVoxelIndex(neighborX, neighborY, neighborZ, chunkSize)])
                            {
                                renderFace = false; // 隣接ボクセルがあるので、この面は不要
                            }
                        }
                        else // 隣接ボクセルが現在のチャンクの境界外にある場合 (つまり、隣接チャンク内か世界の端)
                        {
                            const Chunk* adjacentChunk = nullptr;
                            int relativeNeighborX = neighborX; // 隣接チャンク内での相対座標
                            int relativeNeighborY = neighborY;
                            int relativeNeighborZ = neighborZ;

                            // どの方向の隣接チャンクをチェックするかを判断し、隣接チャンク内の相対座標を決定
                            if (offset.x == -1) { // X-方向
                                adjacentChunk = neighbor_neg_x;
                                relativeNeighborX = chunkSize - 1; // 隣接チャンクの右端
                            } else if (offset.x == 1) { // X+方向
                                adjacentChunk = neighbor_pos_x;
                                relativeNeighborX = 0; // 隣接チャンクの左端
                            } else if (offset.y == -1) { // Y-方向
                                adjacentChunk = neighbor_neg_y;
                                relativeNeighborY = chunkSize - 1; // 隣接チャンクの上端
                            } else if (offset.y == 1) { // Y+方向
                                adjacentChunk = neighbor_pos_y;
                                relativeNeighborY = 0; // 隣接チャンクの下端
                            } else if (offset.z == -1) { // Z-方向
                                adjacentChunk = neighbor_neg_z;
                                relativeNeighborZ = chunkSize - 1; // 隣接チャンクの奥端
                            } else if (offset.z == 1) { // Z+方向
                                adjacentChunk = neighbor_pos_z;
                                relativeNeighborZ = 0; // 隣接チャンクの手前端
                            }
                            
                            // 隣接チャンクが存在し、その中の該当位置にボクセルが存在するかチェック
                            if (adjacentChunk != nullptr && 
                                adjacentChunk->getVoxels()[getVoxelIndex(relativeNeighborX, relativeNeighborY, relativeNeighborZ, chunkSize)])
                            {
                                renderFace = false; // 隣接チャンクにボクセルがあるので、この面は不要
                            }
                            // adjacentChunk が nullptr の場合（世界の端、またはチャンクがロードされていない）、renderFace は true のまま (面を生成する)
                        }

                        if (renderFace)
                        {
                            glm::vec3 currentFaceNormal = faceNormals[i]; // この面の法線を取得

                            for (int v_idx = 0; v_idx < 4; ++v_idx) // 4つの頂点についてループ
                            {
                                unsigned int baseIdx = cubeFaceBaseIndices[i][v_idx];
                                Vertex baseVertex = baseCubeVertices[baseIdx];

                                Vertex newVertex = baseVertex;
                                newVertex.x += x;
                                newVertex.y += y;
                                newVertex.z += z;

                                newVertex.u = faceUVs[v_idx].x;
                                newVertex.v = faceUVs[v_idx].y;

                                newVertex.nx = currentFaceNormal.x;
                                newVertex.ny = currentFaceNormal.y;
                                newVertex.nz = currentFaceNormal.z;

                                meshData.vertices.push_back(newVertex);
                            }

                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 0);
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 1);
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 2);

                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 0);
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 2);
                            meshData.indices.push_back(static_cast<unsigned int>(currentVertexCount) + 3);

                            currentVertexCount += 4;
                        }
                    }
                }
            }
        }
    }
    return meshData;
}