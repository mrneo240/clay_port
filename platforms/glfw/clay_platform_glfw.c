#define GLAD_GL_IMPLEMENTATION
#include "clay_platform.h"

static GLFWwindow* _clay_window;

static void key_callback(GLFWwindow* window, int key, int scancode, int action,
                         int mods) {
  if (key == GLFW_KEY_ESCAPE &&
      (action == GLFW_RELEASE || action == GLFW_PRESS))
    glfwSetWindowShouldClose(window, GL_TRUE);
}

void Clay_Platform_Initialize(int width, int height,
                              const char* title) { /* Initialize the library */
  if (!glfwInit()) return;

  /* Create a windowed mode window and its OpenGL context */
  _clay_window = glfwCreateWindow(width, height, title, NULL, NULL);
  if (!_clay_window) {
    glfwTerminate();
    return;
  }

  /* Make the window's context current */
  glfwMakeContextCurrent(_clay_window);

  gladLoadGL();

  glfwSetKeyCallback(_clay_window, key_callback);
}

void Clay_Platform_Render_Start() {}

void Clay_Platform_Render_End() {
  glfwSwapBuffers(_clay_window);
  glfwPollEvents();
}

bool Clay_Platform_ShouldClose() { return glfwWindowShouldClose(_clay_window); }

void Clay_Platform_Shutdown() {
  glfwDestroyWindow(_clay_window);
  glfwTerminate();
}
