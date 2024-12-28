#include <intraFont.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "clay_platform.h"
#include "clay_renderer.h"

static int _clay_screenHeight = 0;
static int _clay_screenWidth = 0;

intraFont* font = NULL;

typedef float clay_scalar_t;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} Color;

typedef struct {
  int x;
  int y;
  int width;
  int height;
} Rectangle;

typedef struct rgba_t {
  uint8_t r, g, b, a;
} clay_rgba_t;

typedef struct {
  clay_scalar_t x, y;
} clay_vec2_t;

typedef struct {
  clay_scalar_t x, y, z;
} clay_vec3_t;

typedef struct {
  int16_t x, y, z;
} clay_vec3_16_t;

typedef struct {
  clay_vec2_t uv;
  clay_rgba_t color;
  clay_vec3_t pos;
} clay_vertex_t __attribute__((packed));

typedef struct {
  clay_rgba_t color;
  clay_vec3_t pos;
} clay_color_vertex_t __attribute__((packed));

typedef struct {
  uint16_t color;
  clay_vec3_16_t pos;
} clay_color_vertex_16_t __attribute__((packed));

typedef struct {
  clay_vertex_t vertices[3];
} clay_tris_t;

typedef enum { CUSTOM_LAYOUT_ELEMENT_TYPE_NULL } CustomLayoutElementType;

typedef struct {
} CustomLayoutElement_Null;

typedef struct {
  CustomLayoutElementType type;
  union {
    CustomLayoutElement_Null null;
  };
} CustomLayoutElement;

#define CLAY_RECTANGLE_TO_GU_RECTANGLE(rectangle)                 \
  (Rectangle) {                                                   \
    .x = rectangle.x, .y = rectangle.y, .width = rectangle.width, \
    .height = rectangle.height                                    \
  }
#define CLAY_COLOR_TO_GU_COLOR(color)                                         \
  (clay_rgba_t) {                                                             \
    .r = (unsigned char)roundf(color.r), .g = (unsigned char)roundf(color.g), \
    .b = (unsigned char)roundf(color.b), .a = (unsigned char)roundf(color.a)  \
  }

#define GU_COLOR_TO_GU_COLOR_16(c)                                \
  (uint16_t)(((c.a >> 4) & 0xF) << 12 | ((c.b >> 4) & 0xF) << 8 | \
             ((c.g >> 4) & 0xF) << 4 | ((c.r >> 4) & 0xF))

#define RGBA_TO_GU_COLOR_16(c)                                \
  (uint16_t)(((c >> 4) & 0xF) << 12 | ((c >> 4) & 0xF) << 8 | \
             ((c >> 4) & 0xF) << 4 | ((c >> 4) & 0xF))
/* ARGB */
#define WHITE (0xFFFFFFFF)
#define CLEAR (0x00FFFFFF)
#define GRAY (0xFF7F7F7F)
#define BLACK (0xFF000000)
#define RED (0xFF0000FF)

/*
static uint8_t vertMem[1024 * 1024] = {};
static size_t vertexOffset = 0;
void* _sceGuGetMemory(size_t size) {
  void* ptr = vertMem + vertexOffset;
  vertexOffset += size;

  // Uncached
  return (intptr_t)ptr | 0x40000000;
}*/

void DrawRectangle(int posX, int posY, int width, int height,
                   clay_rgba_t color) {
  clay_color_vertex_16_t* vertices = (clay_color_vertex_16_t*)sceGuGetMemory(
      2 * sizeof(clay_color_vertex_16_t));

  uint16_t c = GU_COLOR_TO_GU_COLOR_16(color);

  vertices[0] = (clay_color_vertex_16_t){.color = c,
                                         .pos = {
                                             .x = posX,
                                             .y = posY,
                                             .z = 0,
                                         }};

  vertices[1] = (clay_color_vertex_16_t){.color = c,
                                         .pos = {
                                             .x = posX + width,
                                             .y = posY + height,
                                             .z = 0,
                                         }};

  sceGuDrawArray(GU_SPRITES, GU_COLOR_4444 | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                 2, 0, vertices);
}

#define PI ((float)M_PI)
#define SMOOTH_CIRCLE_ERROR_RATE (0.5f)
void DrawRoundedRect(int x, int y, int width, int height, float cornerRadius,
                     int segments, clay_rgba_t color) {
  if (cornerRadius >= 1.f) cornerRadius = 1.f;

  // hardcode segments to small number, screen is tiny ya know?
  segments = 6;

  // harcoded for now
  clay_color_vertex_t* vertices =
      (clay_color_vertex_t*)sceGuGetMemory(114 * sizeof(clay_color_vertex_t));

  Rectangle rec = (Rectangle){.x = x, .y = y, .width = width, .height = height};

  // Calculate corner radius
  float radius = (rec.width > rec.height) ? (rec.height * cornerRadius) / 2.f
                                          : (rec.width * cornerRadius) / 2.f;
  if (radius <= 0.f) return;
#if 0
    // Calculate number of segments to use for the corners
    if (segments < 4) {
      // Calculate the maximum angle between segments based on the error rate
      // (usually 0.5f)
      float th =
          acosf(2.f * powf(1 - SMOOTH_CIRCLE_ERROR_RATE / radius, 2.f) - 1.f);
      segments = (int)(ceilf(2.f * PI / th) / 4.f);
      if (segments <= 0) segments = 4;
    }
#endif

  float stepLength = 90.0f / (float)segments;

  // Coordinates of the 12 points that define the rounded rect
  const clay_vec2_t point[12] = {
      {(float)rec.x + radius, rec.y},
      {(float)(rec.x + rec.width) - radius, rec.y},
      {rec.x + rec.width, (float)rec.y + radius},  // PO, P1, P2
      {rec.x + rec.width, (float)(rec.y + rec.height) - radius},
      {(float)(rec.x + rec.width) - radius, rec.y + rec.height},  // P3, P4
      {(float)rec.x + radius, rec.y + rec.height},
      {rec.x, (float)(rec.y + rec.height) - radius},
      {rec.x, (float)rec.y + radius},  // P5, P6, P7
      {(float)rec.x + radius, (float)rec.y + radius},
      {(float)(rec.x + rec.width) - radius, (float)rec.y + radius},  // P8, P9
      {(float)(rec.x + rec.width) - radius,
       (float)(rec.y + rec.height) - radius},
      {(float)rec.x + radius, (float)(rec.y + rec.height) - radius}  // P10, P11
  };

  const clay_vec2_t centers[4] = {point[8], point[9], point[10], point[11]};
  const float angles[4] = {180.f, 270.f, 0.f, 90.f};

  int vertexIndex = 0;
  for (int corner = 0; corner < 4; corner++) {  // corner 4
    // Center of the corner arc
    const float centerX = centers[corner].x;
    const float centerY = centers[corner].y;

    // Generate vertices for each segment of the corner
    for (int i = 0; i < segments; i++) {
      float angle =
          angles[corner] + i * stepLength;  // Convert degrees to radians
      float radians = angle * PI / 180.f;
      float angleNext =
          angles[corner] + (i + 1) * stepLength;  // Convert degrees to radians
      float radiansNext = angleNext * PI / 180.f;

      // Calculate vertex coordinates
      vertices[vertexIndex++] = (clay_color_vertex_t){
          .color = color,
          .pos = {.x = centerX + radius * cosf(radians),
                  .y = centerY + radius * sinf(radians),
                  .z = 0},

      };
      vertices[vertexIndex++] = (clay_color_vertex_t){
          .color = color,
          .pos = {.x = centerX + radius * cosf(radiansNext),
                  .y = centerY + radius * sinf(radiansNext),
                  .z = 0},
      };
      vertices[vertexIndex++] = (clay_color_vertex_t){
          .color = color,
          .pos = {.x = centerX, .y = centerY, .z = 0},
      };
    }
  }

  // Rectangles
  // top
  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[0].x, .y = point[0].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[1].x, .y = point[1].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[8].x, .y = point[8].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[8].x, .y = point[8].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[1].x, .y = point[1].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[9].x, .y = point[9].y, .z = 0},
  };
  // middle
  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[7].x, .y = point[7].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[2].x, .y = point[2].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[6].x, .y = point[6].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[6].x, .y = point[6].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[2].x, .y = point[2].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[3].x, .y = point[3].y, .z = 0},
  };
  // bottom
  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[11].x, .y = point[11].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[10].x, .y = point[10].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[5].x, .y = point[5].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[5].x, .y = point[5].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[10].x, .y = point[10].y, .z = 0},
  };

  vertices[vertexIndex++] = (clay_color_vertex_t){
      .color = color,
      .pos = {.x = point[4].x, .y = point[4].y, .z = 0},
  };

  sceGuDisable(GU_DEPTH_TEST);
  sceGuDrawArray(GU_TRIANGLES,
                 GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                 vertexIndex, 0, vertices);
  sceGuEnable(GU_DEPTH_TEST);
}

void DrawRing(Clay_Vector2 center, float innerRadius, float outerRadius,
              float startAngle, float endAngle, int segments,
              clay_rgba_t color) {
  float radius = outerRadius;

#if 0
  // Calculate number of segments to use for the corners
  if (segments < 4) {
    // Calculate the maximum angle between segments based on the error rate
    // (usually 0.5f)
    float th = acosf(2 * powf(1 - SMOOTH_CIRCLE_ERROR_RATE / radius, 2) - 1);
    segments = (int)(ceilf(2 * PI / th) / 4.0f);
    if (segments <= 0) segments = 4;
  }
#endif
  segments = 6;

  float stepLength =
      ((int)fabsf(endAngle - startAngle) % 360) / (float)segments;

  // harcoded for now
  clay_color_vertex_t* vertices =
      (clay_color_vertex_t*)sceGuGetMemory(114 * sizeof(clay_color_vertex_t));

  int vertexIndex = 0;
  //  Center of the corner arc
  const float centerX = center.x;
  const float centerY = center.y;

  // Generate vertices for each segment of the corner
  for (int i = 0; i < segments; i++) {
    float angle = startAngle + i * stepLength;  // Convert degrees to radians
    float radians = angle * PI / 180.f;
    float angleNext =
        startAngle + (i + 1) * stepLength;  // Convert degrees to radians
    float radiansNext = angleNext * PI / 180.f;

    // Calculate vertex coordinates
    vertices[vertexIndex++] = (clay_color_vertex_t){
        .color = color,
        .pos = {.x = centerX + radius * cosf(radians),
                .y = centerY + radius * sinf(radians),
                .z = 0},

    };
    vertices[vertexIndex++] = (clay_color_vertex_t){
        .color = color,
        .pos = {.x = centerX + radius * cosf(radiansNext),
                .y = centerY + radius * sinf(radiansNext),
                .z = 0},
    };
    vertices[vertexIndex++] = (clay_color_vertex_t){
        .color = color,
        .pos = {.x = centerX, .y = centerY, .z = 0},
    };
  }

  sceGuDisable(GU_DEPTH_TEST);
  sceGuDrawArray(GU_TRIANGLES,
                 GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
                 vertexIndex, 0, vertices);
  sceGuEnable(GU_DEPTH_TEST);
}

void Clay_Renderer_Initialize(int width, int height, const char* title) {
  if (width > 480) {
    // float widthScale = 480.f / width;
    width = 480;
  }
  if (height > 272) {
    // float heightScale = 480.f / height;
    height = 272;
  }

  Clay_Platform_Initialize(width, height, title);

  intraFontInit();
  /* Dreamcast recommended to use INTRAFONT_CACHE_ASCII as an option but not
   * required */

  font = intraFontLoad("ltn8.pgf", INTRAFONT_CACHE_ASCII);
  if (!font) {
    return;
  }
  intraFontSetStyle(font, 1.f, BLACK, CLEAR, 0.f, INTRAFONT_ALIGN_LEFT);

  _clay_screenWidth = width;
  _clay_screenHeight = height;
}

void Clay_Renderer_Render(Clay_RenderCommandArray renderCommands) {
  Clay_Platform_Render_Start();
  bool isScissorActive = false;

  // vertexOffset = 0;

  for (int j = 0; j < renderCommands.length; j++) {
    Clay_RenderCommand* renderCommand =
        Clay_RenderCommandArray_Get(&renderCommands, j);
    Clay_BoundingBox boundingBox = renderCommand->boundingBox;
    switch (renderCommand->commandType) {
      case CLAY_RENDER_COMMAND_TYPE_TEXT: {
        Clay_String text = renderCommand->text;
        // Font fontToUse =
        //   Raylib_fonts[renderCommand->config.textElementConfig->fontId].font;

        float fontSize = renderCommand->config.textElementConfig->fontSize;
        float scaleSize = fontSize / 16.f;

        Clay_Color color = renderCommand->config.textElementConfig->textColor;
        clay_rgba_t colorStruct = CLAY_COLOR_TO_GU_COLOR(color);
        uint32_t fontColor = 0;
        memcpy(&fontColor, &colorStruct, sizeof(uint32_t));

        const float adjustX = 4.f;
        const float adjustY = renderCommand->boundingBox.height;  // 12.f;

        intraFontSetStyle(font, scaleSize, fontColor, CLEAR, 0.f,
                          INTRAFONT_ALIGN_LEFT);

        intraFontPrintEx(font, boundingBox.x + adjustX, boundingBox.y + adjustY,
                         text.chars, text.length);

        if (isScissorActive) {
          sceGuEnable(GU_SCISSOR_TEST);
        }
        sceGuDisable(GU_TEXTURE_2D);

        break;
      }
      case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
        /*
        Texture2D imageTexture = *(Texture2D
       *)renderCommand->config.imageElementConfig->imageData; DrawTextureEx(
        imageTexture,
        (Vector2){boundingBox.x, boundingBox.y},
        0,
        boundingBox.width / (float)imageTexture.width,
        WHITE);
        */
        break;
      }
      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
        sceGuEnable(GU_SCISSOR_TEST);
        sceGuScissor(
            (int)roundf(boundingBox.x), (int)roundf(boundingBox.y),
            (int)roundf(boundingBox.x) + (int)roundf(boundingBox.width),
            (int)roundf(boundingBox.y) + (int)roundf(boundingBox.height));
        isScissorActive = true;
        break;
      }
      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
        sceGuDisable(GU_SCISSOR_TEST);
        isScissorActive = false;
        break;
      }
      case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
        Clay_RectangleElementConfig* config =
            renderCommand->config.rectangleElementConfig;

        if (config->cornerRadius.topLeft > 0) {
          float radius = (config->cornerRadius.topLeft * 2) /
                         (float)((boundingBox.width > boundingBox.height)
                                     ? boundingBox.height
                                     : boundingBox.width);
          DrawRoundedRect(boundingBox.x, boundingBox.y, boundingBox.width,
                          boundingBox.height, radius, 8,
                          CLAY_COLOR_TO_GU_COLOR(config->color));
        } else {
          DrawRectangle(boundingBox.x, boundingBox.y, boundingBox.width,
                        boundingBox.height,
                        CLAY_COLOR_TO_GU_COLOR(config->color));
        }
        break;
      }
      case CLAY_RENDER_COMMAND_TYPE_BORDER: {
        Clay_BorderElementConfig* config =
            renderCommand->config.borderElementConfig;
        // Left border
        if (config->left.width > 0) {
          DrawRectangle(
              (int)roundf(boundingBox.x),
              (int)roundf(boundingBox.y + config->cornerRadius.topLeft),
              (int)config->left.width,
              (int)roundf(boundingBox.height - config->cornerRadius.topLeft -
                          config->cornerRadius.bottomLeft),
              CLAY_COLOR_TO_GU_COLOR(config->left.color));
        }
        // Right border
        if (config->right.width > 0) {
          DrawRectangle(
              (int)roundf(boundingBox.x + boundingBox.width -
                          config->right.width),
              (int)roundf(boundingBox.y + config->cornerRadius.topRight),
              (int)config->right.width,
              (int)roundf(boundingBox.height - config->cornerRadius.topRight -
                          config->cornerRadius.bottomRight),
              CLAY_COLOR_TO_GU_COLOR(config->right.color));
        }
        // Top border
        if (config->top.width > 0) {
          DrawRectangle(
              (int)roundf(boundingBox.x + config->cornerRadius.topLeft),
              (int)roundf(boundingBox.y),
              (int)roundf(boundingBox.width - config->cornerRadius.topLeft -
                          config->cornerRadius.topRight),
              (int)config->top.width,
              CLAY_COLOR_TO_GU_COLOR(config->top.color));
        }
        // Bottom border
        if (config->bottom.width > 0) {
          DrawRectangle(
              (int)roundf(boundingBox.x + config->cornerRadius.bottomLeft),
              (int)roundf(boundingBox.y + boundingBox.height -
                          config->bottom.width),
              (int)roundf(boundingBox.width - config->cornerRadius.bottomLeft -
                          config->cornerRadius.bottomRight),
              (int)config->bottom.width,
              CLAY_COLOR_TO_GU_COLOR(config->bottom.color));
        }
        if (config->cornerRadius.topLeft > 0) {
          DrawRing(
              (Clay_Vector2){
                  roundf(boundingBox.x + config->cornerRadius.topLeft),
                  roundf(boundingBox.y + config->cornerRadius.topLeft)},
              roundf(config->cornerRadius.topLeft - config->top.width),
              config->cornerRadius.topLeft, 180, 270, 10,
              CLAY_COLOR_TO_GU_COLOR(config->top.color));
        }
        if (config->cornerRadius.topRight > 0) {
          DrawRing((Clay_Vector2){roundf(boundingBox.x + boundingBox.width -
                                         config->cornerRadius.topRight),
                                  roundf(boundingBox.y +
                                         config->cornerRadius.topRight)},
                   roundf(config->cornerRadius.topRight - config->top.width),
                   config->cornerRadius.topRight, 270, 360, 10,
                   CLAY_COLOR_TO_GU_COLOR(config->top.color));
        }
        if (config->cornerRadius.bottomLeft > 0) {
          DrawRing((Clay_Vector2){roundf(boundingBox.x +
                                         config->cornerRadius.bottomLeft),
                                  roundf(boundingBox.y + boundingBox.height -
                                         config->cornerRadius.bottomLeft)},
                   roundf(config->cornerRadius.bottomLeft - config->top.width),
                   config->cornerRadius.bottomLeft, 90, 180, 10,
                   CLAY_COLOR_TO_GU_COLOR(config->bottom.color));
        }
        if (config->cornerRadius.bottomRight > 0) {
          DrawRing(
              (Clay_Vector2){roundf(boundingBox.x + boundingBox.width -
                                    config->cornerRadius.bottomRight),
                             roundf(boundingBox.y + boundingBox.height -
                                    config->cornerRadius.bottomRight)},
              roundf(config->cornerRadius.bottomRight - config->bottom.width),
              config->cornerRadius.bottomRight, 0.1, 90, 10,
              CLAY_COLOR_TO_GU_COLOR(config->bottom.color));
        }
        break;
      }
      case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
        CustomLayoutElement* customElement =
            (CustomLayoutElement*)
                renderCommand->config.customElementConfig->customData;
        if (!customElement) continue;
        switch (customElement->type) {
          default:
            break;
        }

        break;
      }
      default: {
        printf("Error: unhandled render command.");
#ifdef CLAY_OVERFLOW_TRAP
        raise(SIGTRAP);
#endif
        exit(1);
      }
    }
  }

  Clay_Platform_Render_End();
}

Clay_Dimensions IntraFont_MeasureText(Clay_String* text,
                                      Clay_TextElementConfig* config) {
  // Measure string size for Font
  Clay_Dimensions textSize = {0};

  float fontSize = config->fontSize;
  float scaleSize = fontSize / 16.0f;
  font->size = scaleSize;

  textDimen dimen = intraFontMeasureTextEx(font, text->chars, text->length);

  textSize.width = dimen.width;
  textSize.height = dimen.height;
  return textSize;
}

void Clay_Platform_Shutdown();
void Clay_Renderer_Shutdown() {
  intraFontUnload(font);
  intraFontShutdown();

  Clay_Platform_Shutdown();
}
