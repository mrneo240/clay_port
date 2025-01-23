#ifndef CLAY_RENDERER_GL_LEGACY_H
#define CLAY_RENDERER_GL_LEGACY_H

#include <intraFont.h>
#include <stdbool.h>

#include "clay_include.h"
#include "clay_platform.h"

#define FONT_INTRAFONT_LARGE (0)
#define FONT_INTRAFONT_SMALL (1)

void Clay_Renderer_Initialize(int width, int height, const char* title);

void Clay_Renderer_Shutdown();

void Clay_Renderer_Render(Clay_RenderCommandArray renderCommands);

Clay_Dimensions Renderer_MeasureText(Clay_StringSlice text,
                                     Clay_TextElementConfig* config,
                                     uintptr_t userData);

#endif  // CLAY_RENDERER_GL_LEGACY_H
