#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// カメラのデフォルト値
const float YAW = -90.0f; // YAW は通常、ワールドのZ軸に沿って始まる
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 60.0f; // デフォルトの視野角

// カメラの移動方向を定義する列挙型
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

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
    // カメラの設定
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom; // この値は固定されます

    // コンストラクタ（デフォルト値付き）
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW,
           float pitch = PITCH);

    // ビュー行列を返す
    glm::mat4 getViewMatrix();

    // キーボード入力の状態に基づいて移動するメソッド
    void processMovementVector(bool forward, bool backward, bool left, bool right, float deltaTime);

    // マウス入力に基づいてカメラのオイラー角とベクトルを更新する
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);


private:
    // カメラのベクトルを再計算する (YawとPitchが変更されたときに呼ばれる)
    void updateCameraVectors();
};

#endif // CAMERA_HPP
