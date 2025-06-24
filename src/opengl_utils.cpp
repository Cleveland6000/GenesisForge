#include <glad/glad.h>
#include "opengl_utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

std::string loadShaderSource(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Could not open shader file: " << filePath << std::endl;
        return "";
    }

    std::stringstream ss;

    ss << file.rdbuf();
    return ss.str();
}

unsigned int compileShader(unsigned int type, const std::string &source)
{
    unsigned int shader = glCreateShader(type);
    const char *src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    checkShaderError(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");

    return shader;
}

unsigned int createShaderProgram(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
{

    std::string vertexSource = loadShaderSource(vertexShaderPath);
    std::string fragmentSource = loadShaderSource(fragmentShaderPath);

    if (vertexSource.empty() || fragmentSource.empty())
    {
        std::cerr << "Failed to load shader source files." << std::endl;
        return 0;
    }

    unsigned int program = glCreateProgram();
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (vs == 0 || fs == 0)
    { // コンパイル失敗時に0が返されるとは限らないが、より安全なチェックを行うべき
      // (checkShaderError内でfalseを返すなど)
        glDeleteProgram(program);
        glDeleteShader(vs); // コンパイルに成功していても念のため削除
        glDeleteShader(fs); // コンパイルに成功していても念のため削除
        return 0;
    }

    // --- ここに抜けていた重要な処理を追加します ---
    glAttachShader(program, vs); // 頂点シェーダーをプログラムにアタッチ
    glAttachShader(program, fs); // フラグメントシェーダーをプログラムにアタッチ
    glLinkProgram(program);      // プログラムをリンク
    glValidateProgram(program);  // プログラムを検証 (デバッグ目的で推奨)
    // ---------------------------------------------
    checkProgramError(program);

    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

void checkShaderError(unsigned int shader, const std::string &type)
{
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Error: " << type << " shader comFpilation failed!\n"
                  << infoLog << std::endl;
    }
}

void checkProgramError(unsigned int program)
{
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Error: Shader program linking failed!\n"
                  << infoLog << std::endl;
    }
}