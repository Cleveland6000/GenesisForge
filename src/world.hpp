#ifndef WORLD_HPP
#define WORLD_HPP

// GLMの実験的な拡張機能を使用するために必要
// glm/gtx/hash.hpp がデュアルクォータニオンの実験的な機能に依存しているため、ここに定義します。
#define GLM_ENABLE_EXPERIMENTAL

#include <memory>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp> // glm::ivec3をstd::mapのキーとして使うためにハッシュ関数を提供します（今回のエラーとは直接関係ないが、Worldがglm/gtxを使う可能性があるので残す）
#include "chunk/chunk.hpp"
#include "chunk_mesh_generator.hpp"
#include "chunk_renderer.hpp"
#include "noise/perlin_noise_2d.hpp" // チャンクの地形生成用

// glm::ivec3 を std::map のキーとして使用するためのカスタム比較関数
// std::map はキーがソート可能である必要があるため、operator< を定義する必要があります。
struct Ivector3Comparator {
    bool operator()(const glm::ivec3& a, const glm::ivec3& b) const {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    }
};

// Worldクラスは、ゲーム世界のチャンクの集合を管理します。
// チャンクのロード、アンロード、メッシュの生成、そしてレンダリングデータの管理を行います。
class World {
public:
    // コンストラクタ: チャンクのサイズ、レンダリング距離、キューブの間隔を初期化します。
    World(int chunkSize, int renderDistance, float cubeSpacing);
    // デストラクタ: 全てのチャンクとOpenGLレンダリングデータを解放します。
    ~World();

    // プレイヤーの位置に基づいてチャンクを更新します（ロード/アンロード）。
    void updateChunks(const glm::vec3& playerPosition);

    // レンダリング可能なチャンクのレンダリングデータとワールド座標のマップを返します。
    // カスタム比較関数を使用するようにマップの型を変更
    const std::map<glm::ivec3, ChunkRenderData, Ivector3Comparator>& getRenderableChunks() const {
        return m_chunkRenderDataMap;
    }

    // 特定のチャンク座標にあるチャンクを取得します。
    Chunk* getChunk(const glm::ivec3& chunkCoord);

private:
    int m_chunkSize;        // 各チャンクのボクセルあたりのサイズ (例: 16x16x16)
    int m_renderDistance;   // プレイヤーから各方向にロードされるチャンクの数
    float m_cubeSpacing;    // ボクセル間の間隔

    // ワールド内のチャンクを格納するマップ。キーはチャンクのグリッド座標です。
    // カスタム比較関数を使用するようにマップの型を変更
    std::map<glm::ivec3, std::unique_ptr<Chunk>, Ivector3Comparator> m_chunks;
    // レンダリング可能なチャンクのOpenGLデータを格納するマップ。
    // チャンクのワールド座標とChunkRenderDataを関連付けます。
    // カスタム比較関数を使用するようにマップの型を変更
    std::map<glm::ivec3, ChunkRenderData, Ivector3Comparator> m_chunkRenderDataMap;

    PerlinNoise2D m_perlinNoise; // 地形生成に使用するパーリンノイズジェネレータ

    // チャンクのボクセルデータを生成します（例: 地形を生成）。
    void generateVoxelData(Chunk& chunk, int chunkX, int chunkZ);
    // 指定されたチャンク座標のメッシュを生成または再生成し、OpenGLバッファを更新します。
    void regenerateChunkMesh(const glm::ivec3& chunkCoord);

    // 指定されたチャンク座標に新しいチャンクをロードします。
    void loadChunk(const glm::ivec3& chunkCoord);
    // 指定されたチャンク座標のチャンクをアンロードし、関連するOpenGLリソースを解放します。
    void unloadChunk(const glm::ivec3& chunkCoord);
};

#endif // WORLD_HPP

