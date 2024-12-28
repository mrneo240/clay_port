#define CLAY_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clay.h"
#include "clay_renderer.h"

Clay_Sizing gameIconSizing = {.height = CLAY_SIZING_FIXED(128),
                              .width = CLAY_SIZING_FIXED(128)};

// Clay_Sizing gameIconSizing = {.height = CLAY_SIZING_FIXED(64),
//                               .width = CLAY_SIZING_FIXED(64)};

Clay_RectangleElementConfig contentBackgroundConfig = {
    .color = {90, 90, 90, 255}, .cornerRadius = {8}};

Clay_Color gameIconColors[] = {{245, 149, 178, 255} /* Mauvelous */,
                               {255, 179, 179, 255} /* Melon */,
                               {255, 241, 186, 255} /* Blond */,
                               {190, 227, 237, 255} /* Powder Blue */,
                               {175, 173, 222, 255} /* Maximum Purple */};

Clay_RectangleElementConfig gameIconBackgroundConfig = {
    .color = {90, 90, 90, 255}, .cornerRadius = {8}};

Clay_Sizing layoutExpand = {.height = CLAY_SIZING_GROW(),
                            .width = CLAY_SIZING_GROW()};

int numGames = 12;
char** gameText;
Clay_String* gameTextStrings;

void RenderHeaderButton(Clay_String text, bool active) {
  if (active) {
    CLAY(CLAY_LAYOUT({.padding = {.x = 16, .y = 8}}),
         CLAY_RECTANGLE({.color = {180, 180, 180, 255}, .cornerRadius = 5})) {
      CLAY_TEXT(text, CLAY_TEXT_CONFIG({.fontId = 0,
                                        .fontSize = 16,
                                        .textColor = {255, 255, 255, 255}}));
    }
  }

  else {
    CLAY(CLAY_LAYOUT({.padding = {.x = 16, .y = 8}}),
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
    CLAY(CLAY_LAYOUT({.padding = {.x = 16, .y = 8}, .sizing = layoutExpand}),
         CLAY_BORDER({.bottom = {.color = color, .width = 2}})) {
      CLAY_TEXT(text, CLAY_TEXT_CONFIG(
                          {.fontId = 0, .fontSize = 20, .textColor = color}));
    }
  } else {
    CLAY(CLAY_LAYOUT({.padding = {.x = 16, .y = 8}, .sizing = layoutExpand})) {
      CLAY_TEXT(text, CLAY_TEXT_CONFIG({.fontId = 0,
                                        .fontSize = 20,
                                        .textColor = {255, 255, 255, 255}}));
    };
  }
}

float mouseWheelX = 0.f;
float mouseWheelY = 0.f;
int scrollDown = true;
int numFrames = 0;
int topMenuOptionSelected = 0;
int leftMenuSelected = 0;
int gameOptionSelected = 0;
int gameSelected = 3;

Clay_RenderCommandArray CreateLayout() {
  if (numFrames % 300 == 0) {
    scrollDown = !scrollDown;
    if (scrollDown) {
      mouseWheelY = 0.2f;
    } else {
      mouseWheelY = -0.2f;
    }
  }

  if (numFrames % 60 == 0) {
    topMenuOptionSelected++;
    if (topMenuOptionSelected >= 5) {
      topMenuOptionSelected = 0;
    }
  }

  if (numFrames % 60 == 0) {
    leftMenuSelected++;
    if (leftMenuSelected >= 26) {
      leftMenuSelected = 0;
    }
  }

  if (numFrames % 120 == 0) {
    gameOptionSelected++;
    if (gameOptionSelected >= 4) {
      gameOptionSelected = 0;
    }
  }

  if (numFrames % 300 == 0) {
    gameSelected++;
    if (gameSelected >= 8) {
      gameSelected = 0;
    }
  }

  numFrames++;
  const Clay_Vector2 mousePosition = (Clay_Vector2){.x = 30, .y = 200};
  Clay_SetPointerState(mousePosition, false);
  Clay_UpdateScrollContainers(false, (Clay_Vector2){mouseWheelX, mouseWheelY},
                              1.f / 60.f);

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

        Clay_String csA = {.chars = "A", .length = 1};
        Clay_String csB = {.chars = "B", .length = 1};
        Clay_String csC = {.chars = "C", .length = 1};
        Clay_String csD = {.chars = "D", .length = 1};
        Clay_String csE = {.chars = "E", .length = 1};
        Clay_String csF = {.chars = "F", .length = 1};
        Clay_String csG = {.chars = "G", .length = 1};
        Clay_String csH = {.chars = "H", .length = 1};
        Clay_String csI = {.chars = "I", .length = 1};
        Clay_String csJ = {.chars = "J", .length = 1};
        Clay_String csK = {.chars = "K", .length = 1};
        Clay_String csL = {.chars = "L", .length = 1};
        Clay_String csM = {.chars = "M", .length = 1};
        Clay_String csN = {.chars = "N", .length = 1};
        Clay_String csO = {.chars = "O", .length = 1};
        Clay_String csP = {.chars = "P", .length = 1};
        Clay_String csQ = {.chars = "Q", .length = 1};
        Clay_String csR = {.chars = "R", .length = 1};
        Clay_String csS = {.chars = "S", .length = 1};
        Clay_String csT = {.chars = "T", .length = 1};
        Clay_String csU = {.chars = "U", .length = 1};
        Clay_String csV = {.chars = "V", .length = 1};
        Clay_String csW = {.chars = "W", .length = 1};
        Clay_String csX = {.chars = "X", .length = 1};
        Clay_String csY = {.chars = "Y", .length = 1};
        Clay_String csZ = {.chars = "Z", .length = 1};

        Clay_String cs[] = {csA, csB, csC, csD, csE, csF, csG, csH, csI,
                            csJ, csK, csL, csM, csN, csO, csP, csQ, csR,
                            csS, csT, csU, csV, csW, csX, csY, csZ};

        for (int i = 0; i < 26; i++) {
          Clay_LayoutConfig buttonLayout = i == leftMenuSelected
                                               ? sidebarButtonLayoutActive
                                               : sidebarButtonLayout;
          CLAY(CLAY_LAYOUT(buttonLayout),
               CLAY_RECTANGLE(
                   {.color = {120, 120, 120, 255}, .cornerRadius = {8}})) {
            CLAY_TEXT(cs[i],
                      CLAY_TEXT_CONFIG({.fontId = 0,
                                        .fontSize = 20,
                                        .textColor = {255, 255, 255, 255}}));
          }
        }
      }

      CLAY(CLAY_ID("MainContent"), /* layoutExpand*/
           CLAY_LAYOUT({.sizing = {.height = CLAY_SIZING_PERCENT(0.8f),
                                   .width = CLAY_SIZING_GROW()},
                        .childGap = 16,
                        .padding = {0, 16},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM}),
           CLAY_SCROLL({.vertical = true}),
           CLAY_RECTANGLE(contentBackgroundConfig)) {
        // Child Elements
        int numPerRow = 3;
        int numRows = numGames / numPerRow;
        for (int row = 0; row < numRows; row++) {
          CLAY(CLAY_IDI("GameIconRow", row),
               CLAY_LAYOUT({.sizing = layoutExpand,
                            .childGap = 16,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            .childAlignment = {.x = CLAY_ALIGN_X_CENTER,
                                               .y = CLAY_ALIGN_Y_CENTER}})) {
            for (int col = 0; col < numPerRow; col++) {
              Clay_BorderElementConfig border = {};
              if ((row * 3) + col == gameSelected) {
                Clay_Border whiteOutline = {.width = 8,
                                            .color = buttonColors[1]};
                border =
                    (Clay_BorderElementConfig){.bottom = whiteOutline,
                                               .left = whiteOutline,
                                               .right = whiteOutline,
                                               .top = whiteOutline,
                                               .cornerRadius = {8, 8, 8, 8}};
              }
              CLAY(CLAY_IDI("GameIcon", (row * numPerRow) + col),
                   CLAY_LAYOUT({.sizing = gameIconSizing,
                                .childAlignment = {.x = CLAY_ALIGN_X_CENTER,
                                                   .y = CLAY_ALIGN_Y_CENTER}}),
                   CLAY_RECTANGLE(
                       {.color = gameIconColors[((row * numPerRow) + col) % 5],
                        .cornerRadius = {16}}),
                   CLAY_BORDER(border)) {
                CLAY_TEXT(gameTextStrings[(row * numPerRow) + col],
                          CLAY_TEXT_CONFIG({.fontId = 0,
                                            .fontSize = 24,
                                            .textColor = {0, 0, 0, 255}}));
                if ((row * 3) + col == gameSelected) {
                  bool LaunchSelected = gameOptionSelected == 0;
                  bool CoverSelected = gameOptionSelected == 1;
                  bool InfoSelected = gameOptionSelected == 2;
                  bool BIOSSelected = gameOptionSelected == 3;
                  Clay_FloatingAttachPoints menuattachCenter = {
                      .parent = CLAY_ATTACH_POINT_CENTER_CENTER};

                  Clay_FloatingAttachPoints menuattachLeft = {
                      .parent = CLAY_ATTACH_POINT_CENTER_CENTER,
                      .element = CLAY_ATTACH_POINT_RIGHT_TOP};

                  Clay_FloatingAttachPoints attach =
                      col == 2 ? menuattachLeft : menuattachCenter;

                  CLAY(CLAY_ID("GameOptionMenu"),
                       CLAY_FLOATING({
                           .attachment = attach,
                           .zIndex = 10,
                       }),
                       CLAY_LAYOUT({.layoutDirection = CLAY_TOP_TO_BOTTOM,
                                    .childGap = 16,
                                    .sizing = {.width = CLAY_SIZING_FIXED(160)},
                                    .padding = {0, 8}}),
                       CLAY_RECTANGLE(
                           {.color = {40, 40, 40, 255}, .cornerRadius = {8}})) {
                    RenderGameOption(CLAY_STRING("Launch"), LaunchSelected,
                                     buttonColors[0]);
                    RenderGameOption(CLAY_STRING("Cover"), CoverSelected,
                                     buttonColors[1]);
                    RenderGameOption(CLAY_STRING("Info"), InfoSelected,
                                     buttonColors[2]);
                    RenderGameOption(CLAY_STRING("Reset to BIOS"), BIOSSelected,
                                     buttonColors[3]);

                    Clay_FloatingAttachPointType markerAttachPoint =
                        col == 2 ? CLAY_ATTACH_POINT_RIGHT_TOP
                                 : CLAY_ATTACH_POINT_LEFT_TOP;
                    CLAY(
                        CLAY_ID("GameOptionMenuCorner"),
                        CLAY_FLOATING(
                            {.attachment =
                                 {.parent = markerAttachPoint,
                                  .element = CLAY_ATTACH_POINT_CENTER_CENTER}}),
                        CLAY_LAYOUT(
                            {.layoutDirection = CLAY_TOP_TO_BOTTOM,
                             .sizing = {.width = CLAY_SIZING_FIXED(24),
                                        .height = CLAY_SIZING_FIXED(24)},
                             .padding = {8, 8}}),
                        CLAY_RECTANGLE({.cornerRadius = {32},
                                        .color = buttonColors[1]})){};
                  };
                }
              }
            }
          }
        }

        // End Child Elements
      }
      // End Child Elements
    }
  }

  /*
  if (numGames > 6) {
    Clay_ScrollContainerData scrollData = Clay_GetScrollContainerData(
        Clay_GetElementId(CLAY_STRING("MainContent")));
    // if (scrollData.found && scrollData.contentDimensions.height == 0.f) {
    //   printf("scrollPosition: (%.2f, %.2f)\n",
    // scrollData.scrollPosition->x,
    //          scrollData.scrollPosition->y);
    //   printf("scrollContainerDimensions: (%.2f, %.2f)\n",
    //          scrollData.scrollContainerDimensions.width,
    //          scrollData.scrollContainerDimensions.height);
    //   printf("contentDimensions: (%.2f, %.2f)\n",
    //          scrollData.contentDimensions.width,
    //          scrollData.contentDimensions.height);
    // }

    if (scrollData.found && scrollData.contentDimensions.height > 0.f) {
      CLAY(CLAY_ID("ScrollBar"),
           CLAY_FLOATING(
               {.offset = {.y = -(scrollData.scrollPosition->y /
                                  scrollData.contentDimensions.height) *
                                scrollData.scrollContainerDimensions.height},
                .zIndex = 1,
                .parentId = Clay_GetElementId(CLAY_STRING("MainContent")).id,
                .attachment = {.element = CLAY_ATTACH_POINT_RIGHT_TOP,
                               .parent = CLAY_ATTACH_POINT_RIGHT_TOP}})) {
        CLAY(
            CLAY_ID("ScrollBarButton"),
            CLAY_LAYOUT(
                {.sizing = {CLAY_SIZING_FIXED(12),
                            CLAY_SIZING_FIXED(
                                (scrollData.scrollContainerDimensions.height /
                                 scrollData.contentDimensions.height) *
                                scrollData.scrollContainerDimensions.height)}}),
            CLAY_RECTANGLE({.cornerRadius = {6},
                            .color = Clay_PointerOver(Clay__HashString(
                                         CLAY_STRING("ScrollBar"), 0, 0))
                                         ? (Clay_Color){100, 100, 140, 150}
                                         : (Clay_Color){120, 120, 160, 150}})) {
        }
      }
    }
  }*/

  /* left menu scrollbar */
  Clay_ScrollContainerData scrollData =
      Clay_GetScrollContainerData(Clay_GetElementId(CLAY_STRING("Sidebar")));
  // if (scrollData.found && scrollData.contentDimensions.height == 0.f) {
  //   printf("scrollPosition: (%.2f, %.2f)\n",
  // scrollData.scrollPosition->x,
  //          scrollData.scrollPosition->y);
  //   printf("scrollContainerDimensions: (%.2f, %.2f)\n",
  //          scrollData.scrollContainerDimensions.width,
  //          scrollData.scrollContainerDimensions.height);
  //   printf("contentDimensions: (%.2f, %.2f)\n",
  //          scrollData.contentDimensions.width,
  //          scrollData.contentDimensions.height);
  // }

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

  return Clay_EndLayout();
}

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
    // Clay_SetMaxMeasureTextCacheWordCount(Clay__maxMeasureTextCacheWordCount *
    // 2);
  }
}

int main(void) {
  // const int screenWidth = 480;
  // const int screenHeight = 272;

  const int screenWidth = 640;
  const int screenHeight = 480;

  uint64_t totalMemorySize = Clay_MinMemorySize();
  Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(
      totalMemorySize, malloc(totalMemorySize));

  Clay_SetMeasureTextFunction(IntraFont_MeasureText);
  Clay_Initialize(clayMemory,
                  (Clay_Dimensions){(float)screenWidth, (float)screenHeight},
                  (Clay_ErrorHandler){HandleClayErrors});
  Clay_Renderer_Initialize(screenWidth, screenHeight,
                           "Clay - GLFW (Legacy) Renderer Example");

  gameText = malloc(numGames * sizeof(char*));
  gameTextStrings = malloc(numGames * sizeof(Clay_String));

  for (int i = 0; i < numGames; i++) {
    gameText[i] = malloc((12 + 1) * sizeof(char));
    snprintf(gameText[i], 12, "Game %2d", i);

    gameTextStrings[i] =
        (Clay_String){.chars = gameText[i], .length = strlen(gameText[i])};
  }

  //--------------------------------------------------------------------------------------
  /* Loop until the user closes the window */
  while (!Clay_Platform_ShouldClose()) {
    UpdateDrawFrame();
  }

  Clay_Renderer_Shutdown();

  return 0;
}
