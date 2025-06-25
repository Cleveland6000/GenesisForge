#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// カメラ定義のための列挙型
enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// デフォルト設定
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 4.5f;
const float SENSITIVITY = 0.05f;
const float ZOOM = 60.0f; // カメラのZoomは固定値

// 抽象カメラクラス
class Camera
{
public:
    // カメラの属性
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // オイラー角
    float Yaw;
    float Pitch;
    // カメラオプション
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom; // FOVの値として使われます

    // コンストラクタ
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);

    // ビュー行列を返す
    glm::mat4 getViewMatrix();

    // WASD入力に基づいてカメラ位置を処理
    void processMovementVector(bool forward, bool backward, bool left, bool right, float deltaTime);
    void processVerticalMovement(bool up, bool down, float deltaTime);

    // マウス入力に基づいてカメラの向きを処理
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // --- 新規追加: カメラの位置を取得するゲッター ---
    glm::vec3 getPosition() const { return Position; }

private:
    // カメラベクトルを更新
    void updateCameraVectors();
};
