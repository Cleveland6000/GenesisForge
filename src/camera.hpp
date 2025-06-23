#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

// カメラの移動方向を定義する列挙型
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// デフォルトのカメラ設定
const float YAW         = -90.0f; // ヨー角 (初期はZ軸負方向を向くため)
const float PITCH       = 0.0f;   // ピッチ角
const float SPEED       = 2.5f;   // カメラ移動速度
const float SENSITIVITY = 0.1f;   // マウス感度
const float ZOOM        = 45.0f;  // ズーム (FOV)

class Camera {
public:
    // カメラ属性
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp; // ワールド座標系での上方向（通常は(0, 1, 0)）

    // Eular Angles
    float Yaw;
    float Pitch;

    // カメラ設定
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // コンストラクタ
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors(); // 初期ベクトルを計算
    }

    // ビュー行列を取得
    glm::mat4 getViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // キーボード入力に基づいてカメラの位置を更新
    void processKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
    }

    // マウス入力に基づいてカメラの方向を更新 (ヨーとピッチ)
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // ピッチが-89度から89度の間に制限されるようにする
        if (constrainPitch) {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        updateCameraVectors(); // 新しいヨーとピッチでベクトルを更新
    }

    // スクロール入力に基づいてズームレベルを更新 (FOV)
    void processMouseScroll(float yoffset) {
        Zoom -= yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f) // 最大ズームを45度 (通常の視野) に設定
            Zoom = 45.0f;
    }

private:
    // オイラー角からカメラのFront, Right, Upベクトルを計算
    void updateCameraVectors() {
        // 新しいFrontベクトルを計算
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // RightとUpベクトルを再計算
        Right = glm::normalize(glm::cross(Front, WorldUp)); // ベクトルを正規化する
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

#endif