#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;
in vec3 Normal;
in float AO; // 頂点シェーダーから受け取るAO値

uniform sampler2D ourTexture;
uniform vec3 lightDir;
uniform float ambientStrength;

void main()
{
    vec3 texColor = texture(ourTexture, TexCoord).rgb;

    // AO値を0-1の範囲に正規化し、非線形に変換して環境光に適用
    // pow(x, 2.0) を使用することで、AOが低い（影が濃い）場合に、より強く環境光を減衰させます。
    // AO = 0 -> 0.0 (最も暗い)
    // AO = 1 -> (1/3)^2 = 約0.11
    // AO = 2 -> (2/3)^2 = 約0.44
    // AO = 3 -> 1.0 (最も明るい)
    float ambientOcclusionFactor = pow(AO / 3.0, 2.0); // ここを変更

    // 環境光 (Ambient Light) にAOを適用
    vec3 ambient = ambientStrength * texColor * ambientOcclusionFactor;

    // 拡散反射光 (Diffuse Light)
    float diff = max(dot(Normal, -lightDir), 0.0);
    vec3 diffuse = diff * texColor;

    // 最終的な色を計算 (環境光 + 拡散反射光)
    FragColor = vec4(ambient + diffuse, 1.0);
}