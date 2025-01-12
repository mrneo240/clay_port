#ifndef CLAY_RENDERER_PVR_H
#define CLAY_RENDERER_PVR_H

#include <clay.h>
#include <stdbool.h>

#include "clay_platform.h"

#define FONT_INTRAFONT_LARGE (0)
#define FONT_INTRAFONT_SMALL (0)

void Clay_Renderer_Initialize(int width, int height, const char* title);

void Clay_Renderer_Shutdown();

void Clay_Renderer_Render(Clay_RenderCommandArray renderCommands);

Clay_Dimensions Renderer_MeasureText(Clay_String* text,
                                      Clay_TextElementConfig* config);

#endif  // CLAY_RENDERER_PVR_H
