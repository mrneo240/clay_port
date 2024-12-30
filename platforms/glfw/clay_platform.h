#ifndef CLAY_PLATFORM_GLFW_H
#define CLAY_PLATFORM_GLFW_H

#include <stdbool.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "input.h"

void Clay_Platform_Initialize(int width, int height, const char* title);

void Clay_Platform_Render_Start();

void Clay_Platform_Render_End();

bool Clay_Platform_ShouldClose();

#endif  // CLAY_PLATFORM_GLFW_H
