#include <string.h>

#define GLAD_GL_IMPLEMENTATION
#include "clay_platform.h"

static GLFWwindow* _clay_window;
static inputs _input;
static uint8_t axes_x = 128;
static uint8_t axes_y = 128;

static void key_callback(GLFWwindow* window, int key, int scancode, int action,
                         int mods) {
  if (key == GLFW_KEY_ESCAPE &&
      (action == GLFW_RELEASE || action == GLFW_PRESS))
    glfwSetWindowShouldClose(window, GL_TRUE);
}

static void processInput(GLFWwindow* window) {
  /*  Reset Everything */
  memset(&_input, 0, sizeof(inputs));

  /* Mark out DPAD inputs */
  if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
    _input.dpad |= (DPAD_LEFT);
  }
  if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    _input.dpad |= (DPAD_RIGHT);
  }
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
    _input.dpad |= (DPAD_DOWN);
  }
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
    _input.dpad |= (DPAD_UP);
  }

  /* Mark out Buttons */
  if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
    _input.btn_a = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
    _input.btn_b = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
    _input.btn_x = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
    _input.btn_y = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS) {
    _input.btn_start = 1;
  }

  /* Mark out DPAD inputs */
  if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
    if (axes_x > 0) axes_x--;
  }
  if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
    if (axes_x < 255) axes_x++;
  }

  if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
    if (axes_y > 0) axes_y--;
  }
  if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
    if (axes_y < 255) axes_y++;
  }

  _input.axes_1 = axes_x;
  _input.axes_2 = axes_y;

  /* Triggers */
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
    _input.trg_left = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
    _input.trg_right = 1;
  }

  INPT_ReceiveFromHost(_input);
}

void Clay_Platform_Initialize(int width, int height, const char* title) {
  /* Initialize the library */
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

void Clay_Platform_Render_Start() { processInput(_clay_window); }

void Clay_Platform_Render_End() {
  glfwSwapBuffers(_clay_window);
  glfwPollEvents();
}

bool Clay_Platform_ShouldClose() { return glfwWindowShouldClose(_clay_window); }

void Clay_Platform_Shutdown() {
  glfwDestroyWindow(_clay_window);
  glfwTerminate();
}
