#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

// External Libraries
#define ZROA_NO_SHORT_NAMES
#include "libzro/libzro.h"
#define BHASH_IMPLEMENTATION
#define BHASH_REALLOC(pltr, size, ctx) _zroa_arena_realloc(ptr, size, size, ctx)
#include "bullno1/bhash.h"
#include "k-hash/khash.h"

#define INIT_KEY_HASH(x)       \
  do {                         \
    x = khash32((uint32_t)&x); \
  } while (0)

#include "clay_renderer.h"

#if defined(PSP)
#define iconSize (96)
#else
#define iconSize (128)
#endif

Clay_Sizing gameIconSizing = {.height = CLAY_SIZING_FIXED(iconSize),
                              .width = CLAY_SIZING_FIXED(iconSize)};

static const int numGames = 24;
static const int numGamesPerRow = 3;
// Clay_Sizing gameIconSizing = {
//     .height = CLAY_SIZING_FIXED(iconSize),
//     .width = CLAY_SIZING_PERCENT(1.f / numGamesPerRow)};

Clay_RectangleElementConfig contentBackgroundConfig = {
    .color = {90, 90, 90, 255}, .cornerRadius = {8}};

Clay_Color gameIconColors[] = {{245, 149, 178, 255} /* Mauvelous */,
                               {255, 179, 179, 255} /* Melon */,
                               {255, 241, 186, 255} /* Blond */,
                               {190, 227, 237, 255} /* Powder Blue */,
                               {175, 173, 222, 255} /* Maximum Purple */};

Clay_RectangleElementConfig gameIconBackgroundConfig = {
    .color = {90, 90, 90, 255}, .cornerRadius = {8}};

static int numFrames = 0;

// This is currently broken upstream :(
#if defined(PLATFORM_PSP)
static const Clay_Sizing layoutExpand = {
    .height = (Clay_SizingAxis){.size = {0}, .type = 1},
    .width = (Clay_SizingAxis){.size = {0}, .type = 1}};
#else
// This should be what we use
static const Clay_Sizing layoutExpand = {.height = CLAY_SIZING_GROW(),
                                         .width = CLAY_SIZING_GROW()};
#endif

char **gameText;
Clay_String *gameTextStrings;

// UI State Keys
static uint32_t ui_key_topMenuOptionSelected = 0;
static uint32_t ui_key_gameSelected = 0;
static uint32_t ui_key_gameOptionSelected = 0;
static uint32_t ui_key_gameOptionOpen = 0;

typedef struct {
  union {
    unsigned int ui32;
    int i32;
    float f32;
  } value;
} ui_state_poly;

// Test of trying to do buffered state
typedef BHASH_TABLE(uint32_t, ui_state_poly) ui_state_buffer;
typedef struct {
  ui_state_buffer state_buffer_a, state_buffer_b;
  ui_state_buffer *current_state_buffer, *previous_state_buffer;
  zroa_arena_t state_arena_a, state_arena_b;
} ui_state_ctx;

ui_state_ctx local_ui_ctx = {0};

// padding is left right top bottom
const Clay_Padding consistentPadding =
    (Clay_Padding){.left = 16, .right = 16, .top = 8, .bottom = 8};

void RenderHeaderButton(Clay_String text, bool active) {
  if (active) {
    CLAY(CLAY_LAYOUT({.padding = consistentPadding}),
         CLAY_RECTANGLE({.color = {180, 180, 180, 255}, .cornerRadius = 5})) {
      CLAY_TEXT(text, CLAY_TEXT_CONFIG({.fontId = 0,
                                        .fontSize = 16,
                                        .textColor = {255, 255, 255, 255}}));
    }
  }

  else {
    CLAY(CLAY_LAYOUT({.padding = consistentPadding}),
         CLAY_RECTANGLE({.color = {140, 140, 140, 255}, .cornerRadius = 5})) {
      CLAY_TEXT(text, CLAY_TEXT_CONFIG({.fontId = 0,
                                        .fontSize = 16,
                                        .textColor = {255, 255, 255, 255}}));
    };
  }
}

Clay_Color buttonColors[] = {{255, 153, 153, 255} /* A: red */,
                             {153, 153, 255, 255} /* X: blue */,
                             {255, 255, 153, 255} /* Y: yellow */,
                             {153, 255, 153, 255} /* B: green*/};

void RenderGameOption(Clay_String text, bool active, Clay_Color color) {
  if (active) {
    CLAY(CLAY_LAYOUT({.padding = consistentPadding, .sizing = layoutExpand}),
         CLAY_BORDER({.bottom = {.color = color, .width = 2}})) {
      CLAY_TEXT(text, CLAY_TEXT_CONFIG(
                          {.fontId = 0, .fontSize = 20, .textColor = color}));
    }
  } else {
    CLAY(CLAY_LAYOUT({.padding = consistentPadding, .sizing = layoutExpand})) {
      CLAY_TEXT(text, CLAY_TEXT_CONFIG({.fontId = 0,
                                        .fontSize = 20,
                                        .textColor = {255, 255, 255, 255}}));
    };
  }
}

static void RenderGameOptionMenu(int gameSelected, int row, int col) {
  bhash_index_t index =
      bhash_find(local_ui_ctx.previous_state_buffer, ui_key_gameOptionSelected);
  ui_state_poly poly_gameOptionSelected;
  if (bhash_is_valid(index)) {
    poly_gameOptionSelected = local_ui_ctx.previous_state_buffer->values[index];
  } else {
    poly_gameOptionSelected.value.i32 = 0;
  }
  int gameOptionSelected = poly_gameOptionSelected.value.i32;

  bool LaunchSelected = gameOptionSelected == 0;
  bool CoverSelected = gameOptionSelected == 1;
  bool InfoSelected = gameOptionSelected == 2;
  bool BIOSSelected = gameOptionSelected == 3;
  Clay_FloatingAttachPoints menuattachCenter = {
      .parent = CLAY_ATTACH_POINT_CENTER_CENTER};

  Clay_FloatingAttachPoints menuattachLeft = {
      .element = CLAY_ATTACH_POINT_RIGHT_TOP,
      .parent = CLAY_ATTACH_POINT_CENTER_CENTER,
  };

  Clay_FloatingAttachPoints attach =
      col == 2 ? menuattachLeft : menuattachCenter;

  CLAY(CLAY_ID("GameOptionMenu"),
       CLAY_FLOATING({
           .zIndex = 10,
           .attachment = attach,
       }),
       CLAY_LAYOUT({
           .sizing = {.width = CLAY_SIZING_FIXED(160)},
           .padding = {0, 8},
           .childGap = 16,
           .layoutDirection = CLAY_TOP_TO_BOTTOM,
       }),
       CLAY_RECTANGLE({.color = {40, 40, 40, 255}, .cornerRadius = {8}})) {
    RenderGameOption(CLAY_STRING("Launch"), LaunchSelected, buttonColors[0]);
    RenderGameOption(CLAY_STRING("Cover"), CoverSelected, buttonColors[1]);
    RenderGameOption(CLAY_STRING("Info"), InfoSelected, buttonColors[2]);
    RenderGameOption(CLAY_STRING("Reset to BIOS"), BIOSSelected,
                     buttonColors[3]);

    Clay_FloatingAttachPointType markerAttachPoint =
        col == 2 ? CLAY_ATTACH_POINT_RIGHT_TOP : CLAY_ATTACH_POINT_LEFT_TOP;
    CLAY(CLAY_ID("GameOptionMenuCorner"),
         CLAY_FLOATING({.attachment =
                            {
                                .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                                .parent = markerAttachPoint,
                            }}),
         CLAY_LAYOUT({
             .sizing = {.width = CLAY_SIZING_FIXED(24),
                        .height = CLAY_SIZING_FIXED(24)},
             .padding = {8, 8},
             .layoutDirection = CLAY_TOP_TO_BOTTOM,
         }),
         CLAY_RECTANGLE({
             .color = buttonColors[1],
             .cornerRadius = {32},
         })){};
  };
}

static const Clay_String csA = {.length = 1, .chars = "A"};
static const Clay_String csB = {.length = 1, .chars = "B"};
static const Clay_String csC = {.length = 1, .chars = "C"};
static const Clay_String csD = {.length = 1, .chars = "D"};
static const Clay_String csE = {.length = 1, .chars = "E"};
static const Clay_String csF = {.length = 1, .chars = "F"};
static const Clay_String csG = {.length = 1, .chars = "G"};
static const Clay_String csH = {.length = 1, .chars = "H"};
static const Clay_String csI = {.length = 1, .chars = "I"};
static const Clay_String csJ = {.length = 1, .chars = "J"};
static const Clay_String csK = {.length = 1, .chars = "K"};
static const Clay_String csL = {.length = 1, .chars = "L"};
static const Clay_String csM = {.length = 1, .chars = "M"};
static const Clay_String csN = {.length = 1, .chars = "N"};
static const Clay_String csO = {.length = 1, .chars = "O"};
static const Clay_String csP = {.length = 1, .chars = "P"};
static const Clay_String csQ = {.length = 1, .chars = "Q"};
static const Clay_String csR = {.length = 1, .chars = "R"};
static const Clay_String csS = {.length = 1, .chars = "S"};
static const Clay_String csT = {.length = 1, .chars = "T"};
static const Clay_String csU = {.length = 1, .chars = "U"};
static const Clay_String csV = {.length = 1, .chars = "V"};
static const Clay_String csW = {.length = 1, .chars = "W"};
static const Clay_String csX = {.length = 1, .chars = "X"};
static const Clay_String csY = {.length = 1, .chars = "Y"};
static const Clay_String csZ = {.length = 1, .chars = "Z"};

static const Clay_String cs[] = {csA, csB, csC, csD, csE, csF, csG, csH, csI,
                                 csJ, csK, csL, csM, csN, csO, csP, csQ, csR,
                                 csS, csT, csU, csV, csW, csX, csY, csZ};

static int leftMenuSelected = 0;
static int gameOptionSelected = 0;

static void SetScrollContainerPercent(Clay_ScrollContainerData *scrollData,
                                      float scrollPercent) {
  if (scrollData->found && scrollData->contentDimensions.height > 0.f) {
    float innerHeight = (scrollData->contentDimensions.height -
                         scrollData->scrollContainerDimensions.height);
    scrollData->scrollPosition->y = scrollPercent * innerHeight * -1.f;
  }
}

static void SwapUIBuffers(void) {
  ui_state_buffer *tmp = local_ui_ctx.current_state_buffer;
  local_ui_ctx.current_state_buffer = local_ui_ctx.previous_state_buffer;
  local_ui_ctx.previous_state_buffer = tmp;
  bhash_clear(local_ui_ctx.current_state_buffer);
}

struct Clay_TextElementConfig textCfg = {
    .textColor = (Clay_Color){255.f, 255.f, 255.f, 255.f},
    .fontId = 0,
    .fontSize = 20,
    .letterSpacing = 0,
    .lineHeight = 0};

Clay_RenderCommandArray CreateLayout() {
  SwapUIBuffers();

  // Update top menu animation
  int topMenuOptionSelected = 0;
  {
    bhash_index_t index = bhash_find(local_ui_ctx.previous_state_buffer,
                                     ui_key_topMenuOptionSelected);
    ui_state_poly poly_topMenuOptionsSelected;
    if (bhash_is_valid(index)) {
      poly_topMenuOptionsSelected =
          local_ui_ctx.previous_state_buffer->values[index];
    } else {
      poly_topMenuOptionsSelected.value.i32 = 0;
    }

    topMenuOptionSelected = poly_topMenuOptionsSelected.value.i32;
    if (numFrames % 60 == 0) {
      topMenuOptionSelected++;
      if (topMenuOptionSelected >= 5) {
        topMenuOptionSelected = 0;
      }
    }
    poly_topMenuOptionsSelected.value.i32 = topMenuOptionSelected;
    bhash_put(local_ui_ctx.current_state_buffer, ui_key_topMenuOptionSelected,
              poly_topMenuOptionsSelected);
  }

  if (numFrames % 60 == 0) {
    leftMenuSelected++;
    if (leftMenuSelected >= 26) {
      leftMenuSelected = 0;
    }
  }

  // Update selected game animation
  int gameSelected = 0;
  // First retrieve data from previous frames
  {
    bhash_index_t index =
        bhash_find(local_ui_ctx.previous_state_buffer, ui_key_gameSelected);
    ui_state_poly poly_gameSelected;
    if (bhash_is_valid(index)) {
      poly_gameSelected = local_ui_ctx.previous_state_buffer->values[index];
    } else {
      poly_gameSelected.value.i32 = 0;
    }

    gameSelected = poly_gameSelected.value.i32;

    /* Controller input */
    const int controllerTimeout = 15;
    static int inputLockoutFrames = 0;
    if (INPT_DPAD() && inputLockoutFrames < 0) {
      if (INPT_DPADDirection(DPAD_DOWN)) {
        gameSelected += numGamesPerRow;
      }
      if (INPT_DPADDirection(DPAD_UP)) {
        gameSelected -= numGamesPerRow;
      }
      if (INPT_DPADDirection(DPAD_LEFT)) {
        gameSelected--;
      }
      if (INPT_DPADDirection(DPAD_RIGHT)) {
        gameSelected++;
      }
      gameSelected = MIN(gameSelected, numGames - 1);
      gameSelected = MAX(gameSelected, 0);
      inputLockoutFrames = controllerTimeout;
    }
    inputLockoutFrames--;
    poly_gameSelected.value.i32 = gameSelected;
    bhash_put(local_ui_ctx.current_state_buffer, ui_key_gameSelected,
              poly_gameSelected);
  }

  bool gameOptionOpen = false;
  {
    bhash_index_t index =
        bhash_find(local_ui_ctx.previous_state_buffer, ui_key_gameOptionOpen);
    ui_state_poly poly_gameOptionOpen;
    if (bhash_is_valid(index)) {
      poly_gameOptionOpen = local_ui_ctx.previous_state_buffer->values[index];
    } else {
      poly_gameOptionOpen.value.i32 = 0;
    }

    gameOptionOpen = poly_gameOptionOpen.value.i32 == 1;

    const int controllerTimeout = 1;
    static int inputLockoutFrames = 0;
    if (inputLockoutFrames < 0) {
      if (INPT_Button(BTN_A)) {
        gameOptionOpen = true;
      }
      if (INPT_Button(BTN_B)) {
        gameOptionOpen = false;
      }
      inputLockoutFrames = controllerTimeout;
      fflush(stdout);
    }
    inputLockoutFrames--;

#if 0
    static inputs last_input = {0};
    inputs *cur = INPT_CurrentInput();
    if (memcmp(cur, &last_input, sizeof(inputs))) {
      printf(
          "Input Report: A:%d B:%d X:%d Y:%d Start:%d L:%d R:%d Dpad:%d A1:%d "
          "A2:%d\n",
          cur->btn_a, cur->btn_b, cur->btn_x, cur->btn_y, cur->btn_start,
          cur->trg_left, cur->trg_right, cur->dpad, cur->axes_1, cur->axes_2);
    }
    memcpy(&last_input, cur, sizeof(inputs));
#endif
    poly_gameOptionOpen.value.i32 = gameOptionOpen ? 1 : 0;
    bhash_put(local_ui_ctx.current_state_buffer, ui_key_gameOptionOpen,
              poly_gameOptionOpen);
  }

  {
    bhash_index_t index = bhash_find(local_ui_ctx.previous_state_buffer,
                                     ui_key_gameOptionSelected);
    ui_state_poly poly_gameOptionSelected;
    if (bhash_is_valid(index)) {
      poly_gameOptionSelected =
          local_ui_ctx.previous_state_buffer->values[index];
    } else {
      poly_gameOptionSelected.value.i32 = 0;
    }

    int gameOptionSelected = 0;
    gameOptionSelected = poly_gameOptionSelected.value.i32;
    if (numFrames % 120 == 0) {
      gameOptionSelected++;
      if (gameOptionSelected >= 4) {
        gameOptionSelected = 0;
      }
    }
    poly_gameOptionSelected.value.i32 = gameOptionSelected;
    bhash_put(local_ui_ctx.current_state_buffer, ui_key_gameOptionSelected,
              poly_gameOptionSelected);
  }

  numFrames++;
  /*
  const Clay_Vector2 mousePosition = (Clay_Vector2){.x = 30, .y = 200};
  Clay_SetPointerState(mousePosition, false);
  Clay_UpdateScrollContainers(false, (Clay_Vector2){mouseWheelX, mouseWheelY},
                              1.f / 60.f);
  */

  // start ui here
  Clay_BeginLayout();

  // build ui here
  CLAY(CLAY_ID("OuterContainer"), CLAY_RECTANGLE({.color = {43, 41, 51, 255}}),
       CLAY_LAYOUT({.layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .sizing = layoutExpand,
                    .padding = {8, 8},
                    .childGap = 8})) {
    // Child container elements
    CLAY(CLAY_ID("HeaderBar"),
         CLAY_LAYOUT({.sizing = {.height = CLAY_SIZING_FIXED(60),  // px
                                 .width = CLAY_SIZING_GROW()},
                      .childGap = 16,
                      .padding = {16, 16},
                      .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}}),
         CLAY_RECTANGLE({.color = {90, 90, 90, 255}, .cornerRadius = {8}})) {
      bool fileSelected = topMenuOptionSelected == 0;
      bool editSelected = topMenuOptionSelected == 1;
      bool uploadSelected = topMenuOptionSelected == 2;
      bool mediaSelected = topMenuOptionSelected == 3;
      bool supportSelected = topMenuOptionSelected == 4;

      RenderHeaderButton(CLAY_STRING("File"), fileSelected);
      RenderHeaderButton(CLAY_STRING("Edit"), editSelected);
      CLAY(CLAY_LAYOUT({.sizing = {.width = CLAY_SIZING_GROW()}})){};
      RenderHeaderButton(CLAY_STRING("Upload"), uploadSelected);
      RenderHeaderButton(CLAY_STRING("Media"), mediaSelected);
      RenderHeaderButton(CLAY_STRING("Support"), supportSelected);
    }

    CLAY(CLAY_ID("LowerContent"),
         CLAY_LAYOUT({.sizing = layoutExpand, .childGap = 8})) {
      // Child Elements
      CLAY(CLAY_ID("Sidebar"),
           CLAY_LAYOUT({.sizing = {.width = CLAY_SIZING_FIXED(128),
                                   .height = CLAY_SIZING_GROW()},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .childGap = 8}),
           CLAY_SCROLL({.vertical = true}),
           CLAY_RECTANGLE(contentBackgroundConfig)) {
        Clay_LayoutConfig sidebarButtonLayout = (Clay_LayoutConfig){
            .sizing = {.width = CLAY_SIZING_GROW()}, .padding = {8, 8}};
        Clay_LayoutConfig sidebarButtonLayoutActive = (Clay_LayoutConfig){
            .sizing = {.width = CLAY_SIZING_GROW()}, .padding = {16, 8}};

        for (int i = 0; i < 26; i++) {
          Clay_LayoutConfig buttonLayout = i == leftMenuSelected
                                               ? sidebarButtonLayoutActive
                                               : sidebarButtonLayout;
          CLAY(CLAY_LAYOUT(buttonLayout),
               CLAY_RECTANGLE(
                   {.color = {120, 120, 120, 255}, .cornerRadius = {8}})) {
            CLAY_TEXT(cs[i],
                      CLAY_TEXT_CONFIG({.fontId = 0,
                                        .fontSize = 32,
                                        .textColor = {255, 255, 255, 255}}));
          }
        }
      }

      CLAY(CLAY_ID("MainContent"), /* layoutExpand*/
           CLAY_LAYOUT({.sizing = {.height = CLAY_SIZING_GROW(),
                                   .width = CLAY_SIZING_GROW()},
                        .childGap = 16,
                        .padding = {0, 16},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM}),
           CLAY_SCROLL({.vertical = true}),
           CLAY_RECTANGLE(contentBackgroundConfig)) {
        // Child Elements
        int numRows = numGames / numGamesPerRow;
        for (int row = 0; row < numRows; row++) {
          CLAY(CLAY_IDI("GameIconRow", row),
               CLAY_LAYOUT({.sizing = layoutExpand,
                            .childGap = 16,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            .childAlignment = {.x = CLAY_ALIGN_X_CENTER,
                                               .y = CLAY_ALIGN_Y_CENTER}})) {
            for (int col = 0; col < numGamesPerRow; col++) {
              Clay_BorderElementConfig border = {};
              if ((row * numGamesPerRow) + col == gameSelected) {
                Clay_Border whiteOutline = {
                    .width = 8,
                    .color = buttonColors[1],
                };
                border = (Clay_BorderElementConfig){
                    .bottom = whiteOutline,
                    .left = whiteOutline,
                    .right = whiteOutline,
                    .top = whiteOutline,
                    .cornerRadius = {8, 8, 8, 8},
                };
              }
              CLAY(CLAY_IDI("GameIcon", (row * numGamesPerRow) + col),
                   CLAY_LAYOUT({
                       .sizing = gameIconSizing,
                       .childAlignment = {.x = CLAY_ALIGN_X_CENTER,
                                          .y = CLAY_ALIGN_Y_CENTER},
                   }),
                   CLAY_RECTANGLE({
                       .color =
                           gameIconColors[((row * numGamesPerRow) + col) % 5],
                       .cornerRadius = {16},
                   }),
                   CLAY_BORDER(border)) {
                CLAY_TEXT(gameTextStrings[(row * numGamesPerRow) + col],
                          CLAY_TEXT_CONFIG({
                              .textColor = {0, 0, 0, 255},
                              .fontId = 1,
                              .fontSize = 24,
                          }));
                if (gameOptionOpen) {
                }
                if ((row * numGamesPerRow) + col == gameSelected) {
                  if (gameOptionOpen) {
                    RenderGameOptionMenu(gameSelected, row, col);
                  }
                }
              }
            }
          }
        }  // End GameIconRow
      }  // End MainContent
    }  // End LowerContent
  }

  /* left menu scrollbar */
  Clay_ScrollContainerData scrollData =
      Clay_GetScrollContainerData(Clay_GetElementId(CLAY_STRING("Sidebar")));

  static float scrollPercent = 0.f;
  float scrollAmount = INPT_AnalogF(AXES_Y) / 64.f;
  scrollPercent += scrollAmount;
  if (scrollPercent > 1.f) {
    scrollPercent = 1.f;
  }
  if (scrollPercent < 0.f) {
    scrollPercent = 0.f;
  }

  // scrollPercent = INPT_AnalogF(AXES_Y) * 0.5f + 0.5f;
  SetScrollContainerPercent(&scrollData, scrollPercent);
  /*
  float scrollPercent = 0.f;
  int frame300 = numFrames % 600;
  if (frame300 > 300) {
    frame300 = 600 - frame300;
  }
  scrollPercent = (float)frame300 / 300.f;
  // SetScrollContainerPercent(&scrollData, scrollPercent);
  */

  if (scrollData.found && scrollData.contentDimensions.height > 0.f) {
    CLAY(CLAY_IDI("ScrollBar", 1),
         CLAY_FLOATING(
             {.offset = {.y = -(scrollData.scrollPosition->y /
                                scrollData.contentDimensions.height) *
                              scrollData.scrollContainerDimensions.height},
              .zIndex = 1,
              .parentId = Clay_GetElementId(CLAY_STRING("Sidebar")).id,
              .attachment = {.element = CLAY_ATTACH_POINT_RIGHT_TOP,
                             .parent = CLAY_ATTACH_POINT_RIGHT_TOP}})) {
      CLAY(CLAY_IDI("ScrollBarButton", 1),
           CLAY_LAYOUT(
               {.sizing = {CLAY_SIZING_FIXED(12),
                           CLAY_SIZING_FIXED(
                               (scrollData.scrollContainerDimensions.height /
                                scrollData.contentDimensions.height) *
                               scrollData.scrollContainerDimensions.height)}}),
           CLAY_RECTANGLE({.cornerRadius = {6},
                           .color = Clay_PointerOver(Clay__HashString(
                                        CLAY_STRING("ScrollBar"), 1, 0))
                                        ? (Clay_Color){100, 100, 140, 150}
                                        : (Clay_Color){120, 120, 160, 150}})) {}
    }
  }

  Clay_ScrollContainerData gamesScrollData = Clay_GetScrollContainerData(
      Clay_GetElementId(CLAY_STRING("MainContent")));

  float scrollPercentPerRow =
      numGamesPerRow / (float)(numGames - numGamesPerRow);
  int currentSelectedGameRow = gameSelected / numGamesPerRow;
  SetScrollContainerPercent(&gamesScrollData,
                            currentSelectedGameRow * scrollPercentPerRow);

  // printf("per %.1f row %d scrol %.1f\n", scrollPercentPerRow,
  //        currentSelectedGameRow,
  //        (currentSelectedGameRow * scrollPercentPerRow) * 100.f);

  return Clay_EndLayout();
}

void UpdateDrawFrame(void) {
  Clay_RenderCommandArray renderCommands = CreateLayout();
  Clay_Renderer_Render(renderCommands);
}

void HandleClayErrors(Clay_ErrorData errorData) {
  printf("%s\n", errorData.errorText.chars);
  if (errorData.errorType == CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED) {
    // Clay_SetMaxElementCount(Clay__maxElementCount * 2);
  } else if (errorData.errorType ==
             CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED) {
    // Clay_SetMaxMeasureTextCacheWordCount(Clay__maxMeasureTextCacheWordCount
    // * 2);
  }
}

// static struct Clay_Context claySmallContext = (struct Clay_Context){
//     .maxElementCount = 4096,
//     .maxMeasureTextCacheWordCount = 4096,
//     .internalArena =
//         {
//             .capacity = SIZE_MAX,
//             .memory = (char *)&fakeContext,
//         },
// };

void fixClayLimits(void);

int main(void) {
#if defined(PLATFORM_PSP)
  const int screenWidth = 480;
  const int screenHeight = 272;
#else
  const int screenWidth = 640;
  const int screenHeight = 480;
#endif
  fixClayLimits();

  // Clay_SetCurrentContext(&claySmallContext);
  uint64_t totalMemorySize = Clay_MinMemorySize();
  Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(
      totalMemorySize, malloc(totalMemorySize));

  Clay_Initialize(clayMemory,
                  (Clay_Dimensions){(float)screenWidth, (float)screenHeight},
                  (Clay_ErrorHandler){HandleClayErrors});
  Clay_SetMeasureTextFunction(Renderer_MeasureText, 0);
  Clay_Renderer_Initialize(screenWidth, screenHeight,
                           "Clay - GLFW (Legacy) Renderer Example");

  gameText = (char **)malloc(numGames * sizeof(char *));
  gameTextStrings = (Clay_String *)malloc(numGames * sizeof(Clay_String));

  for (int i = 0; i < numGames; i++) {
    gameText[i] = (char *)malloc((12 + 1) * sizeof(char));
    snprintf(gameText[i], 12, "Game %2d", i);

    gameTextStrings[i] =
        (Clay_String){.length = (int)strlen(gameText[i]), .chars = gameText[i]};
  }

  // Based on the idea from:
  // https://gist.github.com/bullno1/2c7ade6e2112950229c5cb2f88695671
  // and shown
  // https://github.com/bullno1/my-first-story/blob/a11bad208a2d98a4b10ad8bc6db2bfaf851ee4f8/bgame/src/ui.c
  const size_t arena_size = 1024;
  local_ui_ctx.state_arena_a = zroa_arena_init(malloc(arena_size), arena_size);
  local_ui_ctx.state_arena_b = zroa_arena_init(malloc(arena_size), arena_size);

  bhash_config_t config_a = bhash_config_default();
  config_a.memctx = &local_ui_ctx.state_arena_a;
  bhash_config_t config_b = bhash_config_default();
  config_b.memctx = &local_ui_ctx.state_arena_b;

  bhash_reinit(&local_ui_ctx.state_buffer_a, config_a);
  bhash_reinit(&local_ui_ctx.state_buffer_b, config_b);
  bhash_clear(&local_ui_ctx.state_buffer_a);
  bhash_clear(&local_ui_ctx.state_buffer_b);
  local_ui_ctx.current_state_buffer = &local_ui_ctx.state_buffer_a;
  local_ui_ctx.previous_state_buffer = &local_ui_ctx.state_buffer_b;

  /* Setup hashes */
  INIT_KEY_HASH(ui_key_topMenuOptionSelected);
  INIT_KEY_HASH(ui_key_gameSelected);
  INIT_KEY_HASH(ui_key_gameOptionOpen);
  INIT_KEY_HASH(ui_key_gameOptionSelected);

  //--------------------------------------------------------------------------------------
  /* Loop until the user closes the window */
  while (!Clay_Platform_ShouldClose()) {
    UpdateDrawFrame();
  }

  free(local_ui_ctx.state_arena_a.base);
  free(local_ui_ctx.state_arena_b.base);
  Clay_Renderer_Shutdown();

  return 0;
}
