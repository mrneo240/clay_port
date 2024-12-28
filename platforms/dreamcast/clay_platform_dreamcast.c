// Sega Dreamcast
#if !defined(_arch_dreamcast)
#error "Must be using KallistiOS, Platform only valid for Sega Dreamcast!"
#endif

#define CUSTOM_OPENGL_IMPL 1

#include "clay_platform.h"

void
Clay_Platform_Initialize(int width, int height, const char* title)
{
  cont_btn_callback(0,
                    CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                    (cont_btn_callback_t)arch_exit);

  glKosInit();
  glViewport(0, 0, 640, 480);
}

void
Clay_Platform_Render_Start()
{
}

void
Clay_Platform_Render_End()
{
  glKosSwapBuffers();
}

bool
Clay_Platform_ShouldClose()
{
  return false;
}
