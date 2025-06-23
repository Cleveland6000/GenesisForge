#include <GLFW/glfw3.h>
#include "opengl_utils.hpp"

int main()
{
    if (!initializeLibraries())
    {
        return -1;
    }
    GLFWwindow *window = createWindowAndInitializeGlad();
    if (!window)
    {
        return -1;
    }
    runMainLoop(window);
    glfwTerminate();
    return 0;
}