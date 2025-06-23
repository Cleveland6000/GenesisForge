#ifndef OPENGL_UTILS_H
#define OPENGL_UTILS_H

#include <GLFW/glfw3.h>

extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;
extern const float CLEAR_COLOR_R;
extern const float CLEAR_COLOR_G;
extern const float CLEAR_COLOR_B;
extern const float CLEAR_COLOR_A;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
bool initializeLibraries();
GLFWwindow* createWindowAndInitializeGlad();
void runMainLoop(GLFWwindow *window);

#endif // OPENGL_UTILS_H