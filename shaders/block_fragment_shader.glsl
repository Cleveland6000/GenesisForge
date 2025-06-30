#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;
in vec3 Normal;
in float AO; // <--- 頂点シェーダーから受け取るAO値
in vec3 FragPosCameraSpace; // <--- カメラ空間でのフラグメント位置

uniform sampler2D ourTexture;
uniform vec3 lightDir;
uniform float ambientStrength;

// フォグ用のuniform変数
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform float fogDensity; // 指数関数的フォグ用

void main()
{
    vec3 texColor = texture(ourTexture, TexCoord).rgb;

    // AO値を0-1の範囲に正規化し、環境光に適用
    // AO値が0の場合は最も暗く、3の場合は最も明るい (1.0) になるように調整
    // ここでのAOはブロックされていない度合いを示すので、値が大きいほど明るい。
    // そのため、乗算で適用するのが自然。
    float ambientOcclusionFactor = (AO / 3.0); // 0/3, 1/3, 2/3, 3/3 = 0.0, ~0.33, ~0.66, 1.0

    // 環境光 (Ambient Light) にAOを適用
    vec3 ambient = ambientStrength * texColor * ambientOcclusionFactor;

    // 拡散反射光 (Diffuse Light)
    float diff = max(dot(Normal, -lightDir), 0.0);
    vec3 diffuse = diff * texColor;

    vec3 finalColor = ambient + diffuse;

    // フォグの計算
    // カメラからの距離を計算 (FragPosCameraSpace.z は負の値になるので絶対値を取る)
    float fragmentDistance = length(FragPosCameraSpace); 

    // --- ここからフォグの種類を選択 ---

    // 1. リニアフォグ (Linear Fog)
    // float fogFactor = clamp((fogEnd - fragmentDistance) / (fogEnd - fogStart), 0.0, 1.0);

    // 2. 指数関数的フォグ (Exponential Fog) - Minecraftでよく使われる
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2.0)); // fogDensityを調整して濃度を制御
    // float fogFactor = exp(-fragmentDistance * fogDensity); // よりシンプルな指数関数フォグ

    // Clamp fogFactor to be between 0.0 and 1.0
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // 最終的な色にフォグを適用
    // (1.0 - fogFactor) はフォグの「透明度」
    // fogFactor が 1.0 に近いほど元の色が残り、0.0 に近いほどフォグ色になる
    FragColor = vec4(mix(fogColor, finalColor, fogFactor), 1.0);
}
