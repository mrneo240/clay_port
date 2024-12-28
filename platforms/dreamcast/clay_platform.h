#ifndef CLAY_PLATFORM_DREAMCAST_H
#define CLAY_PLATFORM_DREAMCAST_H

#include <kos.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glkos.h>

#include <stdbool.h>

void Clay_Platform_Initialize(int width, int height, const char* title);

void Clay_Platform_Render_Start();

void Clay_Platform_Render_End();

bool Clay_Platform_ShouldClose();

#endif // CLAY_PLATFORM_DREAMCAST_H
