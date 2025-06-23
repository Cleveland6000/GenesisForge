#ifndef OPENGL_UTILS_H
#define OPENGL_UTILS_H

#include <GLFW/glfw3.h>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

std::string loadSHaderSource(const std::string &filePath);
unsigned int compileShader(unsigned int type, const std::string &source);
unsigned int createShaderProgram(const std::string &vertexShaderSource, const std::string &fragmentShaderSource);
void checkShaderError(unsigned int shader, const std::string &type);
void checkProgramError(unsigned int program);

#endif // OPENGL_UTILS_H