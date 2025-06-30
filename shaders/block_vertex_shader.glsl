#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor; // 既存の色属性 (使用しない場合でも互換性のため残す)
layout (location = 2) in vec2 aTexCoord; // テクスチャ座標属性
layout (location = 3) in vec3 aNormal; // 法線属性
layout (location = 4) in float aAO; // <--- AO属性を追加

out vec3 ourColor;
out vec2 TexCoord;
out vec3 Normal;
out float AO; // <--- フラグメントシェーダーへ渡すAO値
out vec3 FragPosCameraSpace; // <--- カメラ空間でのフラグメント位置を追加

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = projection * view * worldPos;
    
    // カメラ空間での位置を計算し、フラグメントシェーダーに渡す
    // フォグの計算には、カメラからの距離が必要なので、カメラ空間での位置が便利
    FragPosCameraSpace = vec3(view * worldPos); 

    ourColor = aColor;
    TexCoord = aTexCoord;
    Normal = normalize(normalMatrix * aNormal);
    AO = aAO; // <--- AO値をそのまま出力
}
