#ifndef CHUNK_RENDERER_HPP
#define CHUNK_RENDERER_HPP

#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>
#include "chunk_mesh_generator.hpp" // ChunkMeshDataの定義用

// ChunkRenderData構造体は、OpenGL描画に必要な情報を保持します。
// VAO, VBO, EBO (またはIBO) および描画するインデックスの数を含みます。
struct ChunkRenderData {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0; // Element Buffer Object (Index Buffer Object)
    GLsizei indexCount = 0; // 描画するインデックスの数
};

class ChunkRenderer {
public:
    // ChunkMeshDataからOpenGLレンダリングデータを生成します。
    // この関数はVAO、VBO、EBOを作成し、データをGPUにアップロードします。
    static ChunkRenderData createChunkRenderData(const ChunkMeshData& meshData);

    // ChunkRenderDataに関連付けられたOpenGLリソースを解放します。
    // VAO、VBO、EBOを削除します。
    static void deleteChunkRenderData(ChunkRenderData& renderData);
};

#endif // CHUNK_RENDERER_HPP

