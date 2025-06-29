#ifndef CHUNK_MANAGER_HPP
#define CHUNK_MANAGER_HPP

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include "chunk/chunk.hpp"
#include "chunk_mesh_generator.hpp"
#include "chunk_renderer.hpp"
#include "terrain_generator.hpp"
#include <array>
#include "ThreadPool.hpp"         // スレッドプールクラス
#include <queue>                  // 非同期メッシュ結果キュー
#include <mutex>                  // スレッド同期のためのミューテックス
#include <unordered_set>          // 保留中のメッシュ生成追跡

// チャンクのワールド座標をキーとするハッシュ関数
// glm::ivec3 を unordered_map のキーとして使用するために必要
struct Vec3iHash
{
    size_t operator()(const glm::ivec3 &v) const
    {
        // X, Y, Z 各成分を組み合わせてハッシュ値を生成
        // シフト演算とXORを組み合わせることで、より均等なハッシュ分布を目指す
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1) ^ (std::hash<int>()(v.z) << 2);
    }
};

class ChunkManager
{
public:
    // コンストラクタ: チャンクと地形生成に関連する設定を受け取る
    ChunkManager(int chunkSize, int renderDistanceXZ, unsigned int noiseSeed, float noiseScale,
                 int worldMaxHeight, int groundLevel, int octaves, float lacunarity, float persistence);
    
    // デストラクタ: リソースのクリーンアップ
    ~ChunkManager();

    // プレイヤーの位置に基づいてチャンクの状態（ロード/アンロード/メッシュ更新）を更新する
    void update(const glm::vec3 &playerPosition);
    
    // 指定されたチャンク座標のチャンクが現在ロードされているかを確認する
    bool hasChunk(const glm::ivec3 &chunkCoord) const;
    
    // 指定されたチャンク座標のチャンクを取得する
    // チャンクが存在しない場合は nullptr を返す
    std::shared_ptr<Chunk> getChunk(const glm::ivec3 &chunkCoord);
    
    // 現在描画可能なすべてのチャンクのレンダリングデータを取得する
    const std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash> &getAllRenderData() const
    {
        return m_chunkRenderData;
    }

private:
    int m_chunkSize;        // 各チャンクのサイズ（ボクセル単位）
    int m_renderDistance;   // プレイヤーからの描画距離（チャンク単位）

    std::unique_ptr<TerrainGenerator> m_terrainGenerator;               // 地形データを生成するオブジェクト
    std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>, Vec3iHash> m_chunks; // ロードされているチャンクのマップ
    std::unordered_map<glm::ivec3, ChunkRenderData, Vec3iHash> m_chunkRenderData; // 描画準備ができたチャンクのOpenGLデータ

    // --- 非同期メッシュ生成関連のメンバ変数 ---
    ThreadPool m_threadPool;                                            // メッシュ生成タスクを実行するためのスレッドプール
    
    // ワーカースレッドからメインスレッドへ、生成されたメッシュデータを安全に渡すためのキュー
    std::queue<std::pair<glm::ivec3, ChunkMeshData>> m_generatedMeshQueue;
    std::mutex m_meshQueueMutex;                                        // m_generatedMeshQueue と m_chunksPendingMeshGeneration を保護するためのミューテックス

    // 現在メッシュ生成がスレッドプールにキューイングされているチャンクを追跡するセット
    // 重複したメッシュ生成要求を防ぐために使用
    std::unordered_set<glm::ivec3, Vec3iHash> m_chunksPendingMeshGeneration;

    // m_chunks マップへのアクセスを保護するためのミューテックス
    // const メソッド内でもロックできるように mutable を使用
    mutable std::mutex m_chunksMutex; 

    // --- プライベートヘルパーメソッド ---
    
    // 指定されたチャンク座標に新しいチャンクを生成し、ボクセルデータを初期化する
    std::shared_ptr<Chunk> generateChunk(const glm::ivec3 &chunkCoord);
    
    // 特定のチャンクのメッシュ生成を非同期で要求する
    // この関数は、バックグラウンドスレッドでのメッシュ生成をトリガーする
    void requestChunkMeshUpdate(const glm::ivec3 &chunkCoord, std::shared_ptr<Chunk> chunk);
    
    // 非同期で生成されたメッシュデータをキューから取り出し、GPUにアップロードする
    // この関数はメインスレッドで実行される必要がある
    void processAsyncMeshResults(); 
    
    // プレイヤーの中心チャンク座標に基づいて、指定された描画距離内のチャンクをロードする
    void loadChunksInArea(const glm::ivec3 &centerChunkCoord);
    
    // 描画距離外に出たチャンクをアンロードし、関連するリソースを解放する
    void unloadDistantChunks(const glm::ivec3 &centerChunkCoord);
    
    // ワールド座標から対応するチャンク座標を計算する
    glm::ivec3 getChunkCoordFromWorldPos(const glm::vec3 &worldPos) const;
    
    glm::ivec3 m_lastPlayerChunkCoord; // 前回のプレイヤーのチャンク座標、更新判定に使用
};

#endif // CHUNK_MANAGER_HPP