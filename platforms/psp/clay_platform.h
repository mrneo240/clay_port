#ifndef CLAY_PLATFORM_PSP_H
#define CLAY_PLATFORM_PSP_H

#include <stdbool.h>
#include "input.h"

void Clay_Platform_Initialize(int width, int height, const char* title);

void Clay_Platform_Render_Start();

void Clay_Platform_Render_End();

bool Clay_Platform_ShouldClose();

#endif // CLAY_PLATFORM_PSP_H
