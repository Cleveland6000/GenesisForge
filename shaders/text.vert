#version 330 core // ここを330 coreに戻す
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(aPos.x, aPos.y, 0.0f, 1.0f);
    TexCoord = aTexCoord;
}