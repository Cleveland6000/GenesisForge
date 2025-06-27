#include "renderer.hpp"
#include <iostream>
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr のために必要

// コンストラクタ
Renderer::Renderer() {
    std::cout << "Renderer: Constructor called.\n";
}

// デストラクタ
Renderer::~Renderer() {
    std::cout << "Renderer: Destructor called.\n";
    // TextRenderer のテクスチャは TextRenderer クラスが管理する
}

// レンダラーの初期化
bool Renderer::initialize(const FontData& fontData) {
    // キューブシェーダーをコンパイル
    m_cubeShader = std::make_unique<Shader>("../assets/shaders/CubeVertexShader.glsl", "../assets/shaders/CubeFragmentShader.glsl");
    if (!m_cubeShader->getID()) {
        std::cerr << "Renderer: Failed to compile cube shader.\n";
        return false;
    }
    std::cout << "Renderer: Cube shader compiled.\n";

    // テキストレンダラーを初期化
    m_textRenderer = std::make_unique<TextRenderer>();
    if (!m_textRenderer->initialize("../assets/shaders/TextVertexShader.glsl", "../assets/shaders/TextFragmentShader.glsl")) {
        std::cerr << "Renderer: Failed to initialize text renderer.\n";
        return false;
    }
    std::cout << "Renderer: Text renderer initialized.\n";

    // フォントアトラステクスチャを設定
    m_fontAtlasTextureID = fontData.atlasTextureID;
    if (m_fontAtlasTextureID == 0) {
        std::cerr << "Renderer: Invalid font atlas texture ID received.\n";
        return false;
    }
    std::cout << "Renderer: Font atlas texture ID set: " << m_fontAtlasTextureID << "\n";

    // OpenGLの設定
    glEnable(GL_DEPTH_TEST); // 深度テストを有効にする
    glEnable(GL_CULL_FACE);  // カリングを有効にする
    glCullFace(GL_BACK);    // バックフェースをカリング

    checkOpenGLError("initialize end");
    std::cout << "Renderer: Initialized successfully.\n";
    return true;
}

// フレームの開始
void Renderer::beginFrame(const glm::vec4& clearColor) {
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // カラーバッファと深度バッファをクリア
    checkOpenGLError("beginFrame");
}

// シーンのレンダリング
void Renderer::renderScene(const glm::mat4 &projection, const glm::mat4 &view, 
                         const ChunkRenderData &chunkRenderData, const glm::vec3& chunkWorldPosition) {
    m_cubeShader->use(); // キューブシェーダーを使用

    // モデル行列を作成: チャンクのワールド位置に平行移動
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, chunkWorldPosition);

    // シェーダーにユニフォーム変数を設定
    m_cubeShader->setMat4("projection", projection);
    m_cubeShader->setMat4("view", view);
    m_cubeShader->setMat4("model", model); // モデル行列を設定

    // チャンクのVAOをバインドして描画コマンドを発行
    glBindVertexArray(chunkRenderData.VAO);
    glDrawElements(GL_TRIANGLES, chunkRenderData.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0); // VAOのバインドを解除

    checkOpenGLError("renderScene");
}

// オーバーレイのレンダリング
void Renderer::renderOverlay(int screenWidth, int screenHeight, const std::string& fpsString, const std::string& positionString) {
    // 2DレンダリングのためにOpenGLの状態を変更
    glDisable(GL_DEPTH_TEST); // 深度テストを無効にする
    glDisable(GL_CULL_FACE);  // カリングも無効にする（2Dテキストには不要）
    glEnable(GL_BLEND);       // ブレンディングを有効にする
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // アルファブレンディングの設定

    m_textRenderer->renderText(m_fontAtlasTextureID, fpsString, 10.0f, static_cast<float>(screenHeight) - 30.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
    m_textRenderer->renderText(m_fontAtlasTextureID, positionString, 10.0f, static_cast<float>(screenHeight) - 60.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));

    // シーンレンダリングのためにOpenGLの状態を元に戻す
    glEnable(GL_DEPTH_TEST); // 深度テストを再度有効にする
    glEnable(GL_CULL_FACE);  // カリングを再度有効にする
    glDisable(GL_BLEND);      // ブレンディングを無効にする
    checkOpenGLError("renderOverlay");
}

// フレームの終了
void Renderer::endFrame() {
    // 特に何もなし（swapBuffersとpollEventsはApplicationクラスが担当）
}

// OpenGLエラーチェック
void Renderer::checkOpenGLError(const std::string& functionName) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL Error in " << functionName << ": " << error << std::endl;
    }
}

