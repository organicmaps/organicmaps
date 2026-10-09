#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

int main()
{
  return glfwPlatformSupported(GLFW_PLATFORM_WAYLAND) && !glfwPlatformSupported(GLFW_PLATFORM_X11) ? 0 : 1;
}
