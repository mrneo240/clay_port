#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

// Clay
#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer.h"

static const int padding = 8;
static const int buttonPadding = 4;

const Clay_Color COLOURS[] = {
    {0, 0, 0, 255},        // background
    {255, 101, 0, 255},    // primary
    {30, 62, 98, 255},     // secondary
    {11, 25, 44, 255},     // tertiary
    {255, 255, 255, 255},  // text
};

const Clay_Color COLOURS_LIGHTENED[] = {
    {51, 51, 51, 255},     // background (0 + 51)
    {255, 121, 51, 255},   // primary (255, 101 + 20, 0 + 51, 255)
    {50, 82, 118, 255},    // secondary (30 + 20, 62 + 20, 98 + 20, 255)
    {31, 45, 64, 255},     // tertiary (11 + 20, 25 + 20, 44 + 20, 255)
    {255, 255, 255, 255},  // text (no change)
};

const int BTN_COLS = 5;
const int BTN_ROWS = 4;

static int current_button = 2;

typedef struct {
  intptr_t id;
  Clay_String label;
  uint8_t colour;
} Button;

typedef struct {
  Button *buttons;
} ButtonRow;

typedef struct {
  ButtonRow *rows;
} ButtonGrid;

float num1 = 0;
float num2 = 0;
bool result = false;
char display[100] = "0";
bool hasPoint = false;
int curOp = -1;

static void HandleButtonInput(void) {
  /* Controller input */
  const int controllerTimeout = 10;
  static int inputLockoutFrames = 0;
  if (INPT_DPAD() && inputLockoutFrames < 0) {
    if (INPT_DPADDirection(DPAD_DOWN)) {
      current_button += BTN_ROWS;
    }
    if (INPT_DPADDirection(DPAD_UP)) {
      current_button -= BTN_ROWS;
    }
    if (INPT_DPADDirection(DPAD_LEFT)) {
      current_button--;
    }
    if (INPT_DPADDirection(DPAD_RIGHT)) {
      current_button++;
    }
    current_button = MIN(current_button, (BTN_COLS * BTN_ROWS) - 1);
    current_button = MAX(current_button, 0);
    inputLockoutFrames = controllerTimeout;
  }
  inputLockoutFrames--;
}

static Clay_PointerData EmulatePointerData(void) {
  Clay_PointerData pointerData = {.position = {-1, -1}, .state = -1};

  const int controllerTimeout = 1;
  static int inputLockoutFrames = 0;
  if (INPT_ButtonEx(BTN_A, BTN_RELEASE)) {
    pointerData.state = CLAY_POINTER_DATA_PRESSED_THIS_FRAME;
    inputLockoutFrames = controllerTimeout;
  }
  inputLockoutFrames--;

  return pointerData;
}

void HandleNumberButtonInteraction(intptr_t id) {
  char tmp[10];
  snprintf(tmp, 10, "%d", id);

  if (strlen(display) < 9) {
    if (strcmp(display, "0") == 0) {
      if (id == 10) {
        strcpy(display, "0.");
        hasPoint = true;
      } else {
        strcpy(display, tmp);
      }
    } else {
      if (id == 10) {
        if (!hasPoint) {
          strcat(display, ".");
          hasPoint = true;
        }
      } else {
        strcat(display, tmp);
      }
    }
  }
}

void HandleEqButtonInteraction() {
  if (curOp != -1) {
    switch (curOp) {
      case 0:
        num1 += num2;
        break;
      case 1:
        num1 -= num2;
        break;
      case 2:
        num1 *= num2;
        break;
      case 3:
        num1 /= num2;
        break;
    }
    snprintf(display, sizeof display, "%g", num1);
    result = true;
  }
#if DEBUG
  printf("%f OP %f = %s\n", num1, num2, display);
#endif
}

void HandleButtonInteraction(Clay_ElementId elementId,
                             Clay_PointerData pointerData, intptr_t id) {
  if (pointerData.state != CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
    return;
  }

  if (id >= 0 && id <= (BTN_COLS * BTN_ROWS) - 1) {
    if (id <= 10) {
      if (!result) {
        HandleNumberButtonInteraction(id);
      }
    }

    switch (id) {
      case 11:
        if (!result) {
          num2 = strtod(display, NULL);
        }
        HandleEqButtonInteraction();
        break;
      case 12 ... 15:
        num1 = strtod(display, NULL);
        result = false;
        strcpy(display, "0");
        curOp = id - 12;
        break;
      case 16:
        num1 = fabs(strtod(display, NULL));
        snprintf(display, sizeof display, "%g", num1);
        break;
      case 17:
        num1 = 1 / strtod(display, NULL);
        snprintf(display, sizeof display, "%g", num1);
        break;
      case 18:
        num1 = 0;
        num2 = 0;
        strcpy(display, "0");
        curOp = -1;
        result = false;
        break;
      case 19:
        if (!result) {
          size_t lastIndex = strlen(display) - 1;
          if (display[lastIndex] == '.') hasPoint = false;
          if (lastIndex == 0) {
            display[lastIndex] = '0';
          } else {
            display[lastIndex] = '\0';
          }
        }
    }
  }
}

Clay_RenderCommandArray CreateLayout(ButtonGrid *grid) {
  // Raylib_fonts[FONT_60_REGULAR] = (Raylib_Font){
  //     .font =
  //         LoadFontEx("resources/AtkinsonHyperlegible-Regular.ttf", 72, 0, 0),
  //     .fontId = FONT_60_REGULAR,
  // };
  // Raylib_fonts[FONT_60_BOLD] = (Raylib_Font){
  //     .font = LoadFontEx("resources/AtkinsonHyperlegible-Bold.ttf", 72, 0,
  //     0), .fontId = FONT_60_BOLD,
  // };
  // Raylib_fonts[FONT_32_REGULAR] = (Raylib_Font){
  //     .font =
  //         LoadFontEx("resources/AtkinsonHyperlegible-Regular.ttf", 32, 0, 0),
  //     .fontId = FONT_32_REGULAR,
  // };
  // Raylib_fonts[FONT_32_BOLD] = (Raylib_Font){
  //     .font = LoadFontEx("resources/AtkinsonHyperlegible-Bold.ttf", 32, 0,
  //     0), .fontId = FONT_32_BOLD,
  // };

  // turn on for debug tools
  // Clay_SetDebugModeEnabled(true);

  // Vector2 mousePosition = GetMousePosition();
  // Vector2 scrollDelta = GetMouseDelta();
  // Clay_SetPointerState((Clay_Vector2){mousePosition.x, mousePosition.y},
  //                      IsMouseButtonDown(0));

  Clay_Sizing layoutExpand = {
      .width = CLAY_SIZING_GROW(),
      .height = CLAY_SIZING_GROW(),
  };

  Clay_BeginLayout();

  CLAY(CLAY_ID("Background"),
       CLAY_RECTANGLE((Clay_RectangleElementConfig){.color = COLOURS[0]}),
       CLAY_LAYOUT((Clay_LayoutConfig){
           .layoutDirection = CLAY_LEFT_TO_RIGHT,
           .sizing = layoutExpand,
       })) {
    CLAY(CLAY_ID("MainContent"),
         CLAY_RECTANGLE((Clay_RectangleElementConfig){.color = COLOURS[0]}),
         CLAY_LAYOUT((Clay_LayoutConfig){
             .layoutDirection = CLAY_TOP_TO_BOTTOM,
             .sizing = layoutExpand,
             .padding = {padding, padding},
             .childGap = padding,
         })) {
      CLAY(CLAY_ID("HeaderContainer"),
           CLAY_RECTANGLE((Clay_RectangleElementConfig){.color = COLOURS[3],
                                                        .cornerRadius = 16}),
           CLAY_LAYOUT((Clay_LayoutConfig){
               .layoutDirection = CLAY_TOP_TO_BOTTOM,
               .sizing = {.height = CLAY_SIZING_FIXED(60),
                          .width = CLAY_SIZING_GROW()},
               .padding = {padding, padding},
               .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}})) {
        CLAY_TEXT(CLAY_STRING("Clayculator"),
                  CLAY_TEXT_CONFIG({
                      .fontId = FONT_INTRAFONT_LARGE,
                      .fontSize = 32,
                      .textColor = COLOURS[4],
                  }));
      }
      CLAY(CLAY_ID("CalcResultContainer"),
           CLAY_RECTANGLE((Clay_RectangleElementConfig){.color = COLOURS[2],
                                                        .cornerRadius = 16}),
           CLAY_LAYOUT((Clay_LayoutConfig){
               .layoutDirection = CLAY_TOP_TO_BOTTOM,
               .sizing = {.height = CLAY_SIZING_FIXED(100),
                          .width = CLAY_SIZING_GROW()},
               .padding = {padding, padding},
               .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}})) {
        CLAY_TEXT(CLAY_STRING(display), CLAY_TEXT_CONFIG({
                                            .fontId = FONT_INTRAFONT_LARGE,
                                            .fontSize = 72,
                                            .textColor = COLOURS[4],
                                        }));
      }
      CLAY(CLAY_ID("InnerContent"),
           CLAY_RECTANGLE((Clay_RectangleElementConfig){.color = COLOURS[0]}),
           CLAY_LAYOUT((Clay_LayoutConfig){
               .layoutDirection = CLAY_LEFT_TO_RIGHT,
               .sizing = (Clay_Sizing){.width = CLAY_SIZING_GROW(),
                                       .height = CLAY_SIZING_GROW()},
               .padding = {padding, padding},
               .childGap = padding,
           })) {
        CLAY(CLAY_ID("LeftPillar"),
             CLAY_RECTANGLE((Clay_RectangleElementConfig){.color = COLOURS[0]}),
             CLAY_LAYOUT((Clay_LayoutConfig){
                 .sizing = (Clay_Sizing){.width = CLAY_SIZING_PERCENT(0.2f)},
             })) {}
        CLAY(CLAY_ID("CalcButtonsContainer"),
             CLAY_LAYOUT(
                 (Clay_LayoutConfig){.sizing = layoutExpand,
                                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                     .childGap = padding})) {
          ButtonGrid buttonGrid = *grid;
          for (int i = 0; i < BTN_COLS; i++) {
            char rowId[10];
            snprintf(rowId, sizeof rowId, "CalcRow%d", i);
            CLAY(CLAY_ID(rowId), CLAY_LAYOUT((Clay_LayoutConfig){
                                     .sizing = layoutExpand,
                                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                     .childGap = 8})) {
              for (int j = 0; j < BTN_ROWS; j++) {
                int buttonIndex = (i * BTN_ROWS) + j;

                char buttonId[16];
                snprintf(buttonId, sizeof buttonId, "CalcButton%d_%d", i, j);

                Button b = buttonGrid.rows[i].buttons[j];
                Clay_Color c = COLOURS[b.colour];
                if (current_button == buttonIndex) {
                  c = COLOURS_LIGHTENED[b.colour];
                }

                CLAY(CLAY_ID(buttonId),
                     CLAY_LAYOUT((Clay_LayoutConfig){
                         .sizing = layoutExpand,
                         .padding = {24, 12},
                         .childAlignment = {.x = CLAY_ALIGN_X_CENTER,
                                            .y = CLAY_ALIGN_Y_CENTER}}),
                     CLAY_RECTANGLE((Clay_RectangleElementConfig){
                         .color = c,
                         .cornerRadius = 16,
                     }),
                     Clay_OnHover(HandleButtonInteraction, b.id)) {
                  CLAY_TEXT(b.label, CLAY_TEXT_CONFIG((Clay_TextElementConfig){
                                         .fontId = FONT_INTRAFONT_SMALL,
                                         .fontSize = 32,
                                         .textColor = COLOURS[4]}));
                }
              }
            }
          }
        }
        CLAY(CLAY_ID("RightPillar"),
             CLAY_RECTANGLE((Clay_RectangleElementConfig){.color = COLOURS[0]}),
             CLAY_LAYOUT((Clay_LayoutConfig){
                 .sizing = (Clay_Sizing){.width = CLAY_SIZING_PERCENT(0.2f)},
             })) {}
      }
    }
  }

  return Clay_EndLayout();
}

Clay_RenderCommandArray CreateLayoutPSP(ButtonGrid *grid) {
  // Raylib_fonts[FONT_60_REGULAR] = (Raylib_Font){
  //     .font =
  //         LoadFontEx("resources/AtkinsonHyperlegible-Regular.ttf", 72, 0,
  //         0),
  //     .fontId = FONT_60_REGULAR,
  // };
  // Raylib_fonts[FONT_60_BOLD] = (Raylib_Font){
  //     .font = LoadFontEx("resources/AtkinsonHyperlegible-Bold.ttf", 72, 0,
  //     0), .fontId = FONT_60_BOLD,
  // };
  // Raylib_fonts[FONT_32_REGULAR] = (Raylib_Font){
  //     .font =
  //         LoadFontEx("resources/AtkinsonHyperlegible-Regular.ttf", 32, 0,
  //         0),
  //     .fontId = FONT_32_REGULAR,
  // };
  // Raylib_fonts[FONT_32_BOLD] = (Raylib_Font){
  //     .font = LoadFontEx("resources/AtkinsonHyperlegible-Bold.ttf", 32, 0,
  //     0), .fontId = FONT_32_BOLD,
  // };

  // turn on for debug tools
  // Clay_SetDebugModeEnabled(true);

  // Vector2 mousePosition = GetMousePosition();
  // Vector2 scrollDelta = GetMouseDelta();
  // Clay_SetPointerState((Clay_Vector2){mousePosition.x, mousePosition.y},
  //                      IsMouseButtonDown(0));

  Clay_Sizing layoutExpand = {
      .width = CLAY_SIZING_GROW(),
      .height = CLAY_SIZING_GROW(),
  };

  Clay_BeginLayout();

  CLAY(CLAY_ID("Background"),
       CLAY_RECTANGLE((Clay_RectangleElementConfig){.color = COLOURS[0]}),
       CLAY_LAYOUT((Clay_LayoutConfig){
           .layoutDirection = CLAY_LEFT_TO_RIGHT,
           .sizing = layoutExpand,
       })) {
    CLAY(CLAY_ID("LeftPillar"),
         CLAY_RECTANGLE((Clay_RectangleElementConfig){.color = COLOURS[0]}),
         CLAY_LAYOUT((Clay_LayoutConfig){
             .sizing = layoutExpand,
             .layoutDirection = CLAY_TOP_TO_BOTTOM,
             .padding = {padding, padding},
             .childGap = padding,
         })) {
      CLAY(CLAY_ID("HeaderContainer"),
           CLAY_RECTANGLE((Clay_RectangleElementConfig){.color = COLOURS[3],
                                                        .cornerRadius = 16}),
           CLAY_LAYOUT((Clay_LayoutConfig){
               .layoutDirection = CLAY_TOP_TO_BOTTOM,
               .sizing = {.height = CLAY_SIZING_FIXED(60),
                          .width = CLAY_SIZING_GROW()},
               .padding = {padding, padding},
               .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}})) {
        CLAY_TEXT(CLAY_STRING("Clayculator"),
                  CLAY_TEXT_CONFIG({
                      .fontId = FONT_INTRAFONT_LARGE,
                      .fontSize = 32,
                      .textColor = COLOURS[4],
                  }));
      }
      CLAY(CLAY_ID("CalcResultContainer"),
           CLAY_RECTANGLE((Clay_RectangleElementConfig){.color = COLOURS[2],
                                                        .cornerRadius = 16}),
           CLAY_LAYOUT((Clay_LayoutConfig){
               .layoutDirection = CLAY_TOP_TO_BOTTOM,
               .sizing = {.height = CLAY_SIZING_FIXED(100),
                          .width = CLAY_SIZING_GROW()},
               .padding = {padding, padding},
               .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}})) {
        CLAY_TEXT(CLAY_STRING(display), CLAY_TEXT_CONFIG({
                                            .fontId = FONT_INTRAFONT_SMALL,
                                            .fontSize = 64,
                                            .textColor = COLOURS[4],
                                        }));
      }
    }
    CLAY(CLAY_ID("MainContent"),
         CLAY_RECTANGLE((Clay_RectangleElementConfig){.color = COLOURS[0]}),
         CLAY_LAYOUT((Clay_LayoutConfig){
             .layoutDirection = CLAY_TOP_TO_BOTTOM,
             .padding = {padding, padding},
             .childGap = padding,
         })) {
      CLAY(
          CLAY_ID("CalcButtonsContainer"),
          CLAY_LAYOUT((Clay_LayoutConfig){.sizing = layoutExpand,
                                          .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                          .childGap = padding})) {
        ButtonGrid buttonGrid = *grid;
        for (int i = 0; i < BTN_COLS; i++) {
          char rowId[10];
          snprintf(rowId, sizeof rowId, "CalcRow%d", i);
          CLAY(CLAY_ID(rowId), CLAY_LAYOUT((Clay_LayoutConfig){
                                   .sizing = layoutExpand,
                                   .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                   .childGap = 8})) {
            for (int j = 0; j < BTN_ROWS; j++) {
              int buttonIndex = (i * BTN_ROWS) + j;

              char buttonId[16];
              snprintf(buttonId, sizeof buttonId, "CalcButton%d_%d", i, j);

              Button b = buttonGrid.rows[i].buttons[j];
              Clay_Color c = COLOURS[b.colour];
              if (current_button == buttonIndex) {
                c = COLOURS_LIGHTENED[b.colour];
              }

              CLAY(CLAY_ID(buttonId),
                   CLAY_LAYOUT((Clay_LayoutConfig){
                       .sizing = layoutExpand,
                       .padding = {padding + 4, padding + 4},
                       .childAlignment = {.x = CLAY_ALIGN_X_CENTER,
                                          .y = CLAY_ALIGN_Y_CENTER}}),
                   CLAY_RECTANGLE((Clay_RectangleElementConfig){
                       .color = c,
                       .cornerRadius = 16,
                   }),
                   Clay_OnHover(HandleButtonInteraction, b.id)) {
                CLAY_TEXT(b.label, CLAY_TEXT_CONFIG((Clay_TextElementConfig){
                                       .fontId = FONT_INTRAFONT_SMALL,
                                       .fontSize = 32,
                                       .textColor = COLOURS[4]}));
              }
            }
          }
        }
      }
    }
    // CLAY(CLAY_ID("RightPillar"),
    //      CLAY_RECTANGLE((Clay_RectangleElementConfig){.color =
    //      COLOURS[0]}), CLAY_LAYOUT((Clay_LayoutConfig){
    //          .sizing = layoutExpand,
    //      })) {}
  }

  return Clay_EndLayout();
}

void UpdateDrawFrame(ButtonGrid *grid) {
  static int last_button = 0;
  HandleButtonInput();
#if defined(PLATFORM_PSP)
  Clay_RenderCommandArray renderCommands = CreateLayoutPSP(grid);
#else
  Clay_RenderCommandArray renderCommands = CreateLayout(grid);
#endif
  Clay_Renderer_Render(renderCommands);

  // Emulate Input
  int i = current_button / BTN_ROWS;
  int j = current_button % BTN_ROWS;
  Button b = grid->rows[i].buttons[j];
  HandleButtonInteraction((Clay_ElementId){0}, EmulatePointerData(), b.id);
  last_button = current_button;
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

  ButtonGrid buttonGrid = (ButtonGrid){
      .rows =
          (ButtonRow[]){
              {
                  .buttons =
                      (Button[]){
                          {.id = 16, .label = CLAY_STRING("|x|"), .colour = 3},
                          {.id = 17, .label = CLAY_STRING("1/x"), .colour = 3},
                          {.id = 18, .label = CLAY_STRING("C"), .colour = 3},
                          {.id = 19, .label = CLAY_STRING("<"), .colour = 3},
                      },
              },
              {
                  .buttons =
                      (Button[]){
                          {.id = 7, .label = CLAY_STRING("7"), .colour = 2},
                          {.id = 8, .label = CLAY_STRING("8"), .colour = 2},
                          {.id = 9, .label = CLAY_STRING("9"), .colour = 2},
                          {.id = 15, .label = CLAY_STRING("/"), .colour = 3},
                      },
              },
              {
                  .buttons =
                      (Button[]){
                          {.id = 4, .label = CLAY_STRING("4"), .colour = 2},
                          {.id = 5, .label = CLAY_STRING("5"), .colour = 2},
                          {.id = 6, .label = CLAY_STRING("6"), .colour = 2},
                          {.id = 14, .label = CLAY_STRING("*"), .colour = 3},
                      },
              },
              {
                  .buttons =
                      (Button[]){
                          {.id = 1, .label = CLAY_STRING("1"), .colour = 2},
                          {.id = 2, .label = CLAY_STRING("2"), .colour = 2},
                          {.id = 3, .label = CLAY_STRING("3"), .colour = 2},
                          {.id = 13, .label = CLAY_STRING("-"), .colour = 3},
                      },
              },
              {
                  .buttons =
                      (Button[]){
                          {.id = 10, .label = CLAY_STRING("."), .colour = 2},
                          {.id = 0, .label = CLAY_STRING("0"), .colour = 2},
                          {.id = 11, .label = CLAY_STRING("="), .colour = 1},
                          {.id = 12, .label = CLAY_STRING("+"), .colour = 3},
                      },
              }},
  };

  uint64_t totalMemorySize = Clay_MinMemorySize();
  Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(
      totalMemorySize, malloc(totalMemorySize));

  Clay_SetMeasureTextFunction(IntraFont_MeasureText);
  Clay_Initialize(clayMemory,
                  (Clay_Dimensions){(float)screenWidth, (float)screenHeight},
                  (Clay_ErrorHandler){HandleClayErrors});
  Clay_Renderer_Initialize(screenWidth, screenHeight, "Clayculator");

  //--------------------------------------------------------------------------------------
  /* Loop until the user closes the window */
  while (!Clay_Platform_ShouldClose()) {
    UpdateDrawFrame(&buttonGrid);
  }

  Clay_Renderer_Shutdown();

  return 0;
}
