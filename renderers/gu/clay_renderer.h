#ifndef CLAY_RENDERER_GU_H
#define CLAY_RENDERER_GU_H

#include <clay.h>
#include <intraFont.h>
#include <stdbool.h>

#include <pspkernel.h>
#include <pspgu.h>
#include <pspdisplay.h>

#include "clay_platform.h"

void Clay_Renderer_Initialize(int width, int height, const char* title);

void Clay_Renderer_Shutdown();

void Clay_Renderer_Render(Clay_RenderCommandArray renderCommands);

Clay_Dimensions IntraFont_MeasureText(Clay_String* text,
                                      Clay_TextElementConfig* config);

#endif  // CLAY_RENDERER_GU_H