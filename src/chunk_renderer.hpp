#ifndef CHUNK_RENDERER_HPP
#define CHUNK_RENDERER_HPP

#include <vector>
#include <glm/glm.hpp> // glm::vec3, glm::vec2 などが必要な場合
#include "chunk_render_data.hpp" // ChunkRenderData の定義をインクルード

// ChunkMeshData の前方宣言
struct ChunkMeshData;

class ChunkRenderer
{
public:
    static ChunkRenderData createChunkRenderData(const ChunkMeshData& meshData);
    // 新しく追加する関数: OpenGLリソースを解放します
    static void deleteChunkRenderData(const ChunkRenderData& data);

    static void renderChunk(const ChunkRenderData& renderData, const glm::vec3& chunkWorldPos);
};

#endif // CHUNK_RENDERER_HPP