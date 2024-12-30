#include <string.h>

// Clay
#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer.h"

void UpdateDrawFrame(void) {
    Clay_RenderCommandArray renderCommands = CreateLayout();
    Clay_Renderer_Render(renderCommands);
}

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
    if (errorData.errorType == CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED) {
        // Clay_SetMaxElementCount(Clay__maxElementCount * 2);
    } else if (errorData.errorType ==
               CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED) {
        // Clay_SetMaxMeasureTextCacheWordCount(Clay__maxMeasureTextCacheWordCount
        // * 2);
    }
}

int main(void) {
#if defined(PLATFORM_PSP)
    const int screenWidth = 480;
    const int screenHeight = 272;
#else
    const int screenWidth = 640;
    const int screenHeight = 480;
#endif

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(
        totalMemorySize, malloc(totalMemorySize));

    Clay_SetMeasureTextFunction(IntraFont_MeasureText);
    Clay_Initialize(clayMemory,
                    (Clay_Dimensions){(float)screenWidth, (float)screenHeight},
                    (Clay_ErrorHandler){HandleClayErrors});
    Clay_Renderer_Initialize(screenWidth, screenHeight,
                             "Clayculator");

    //--------------------------------------------------------------------------------------
    /* Loop until the user closes the window */
    while (!Clay_Platform_ShouldClose()) {
        UpdateDrawFrame();
    }

    Clay_Renderer_Shutdown();

    return 0;
}
