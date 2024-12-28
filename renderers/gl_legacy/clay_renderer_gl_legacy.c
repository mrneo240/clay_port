#include <intraFont.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clay_platform.h"
#include "clay_renderer.h"

#ifndef PATH_ASSETS
#define PATH_ASSETS ""
#endif

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
  uint32_t flags;
  clay_vec3_t pos;
  clay_vec2_t uv;
  clay_rgba_t color;  // bgra
  union {
    float pad;
    uint32_t vertindex;
  } pad0;
} clay_vertex_t;

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

#define CLAY_RECTANGLE_TO_OPENGL_LEGACY_RECTANGLE(rectangle)      \
  (Rectangle) {                                                   \
    .x = rectangle.x, .y = rectangle.y, .width = rectangle.width, \
    .height = rectangle.height                                    \
  }
#define CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(color)                              \
  (clay_rgba_t) {                                                             \
    .r = (unsigned char)roundf(color.r), .g = (unsigned char)roundf(color.g), \
    .b = (unsigned char)roundf(color.b), .a = (unsigned char)roundf(color.a)  \
  }

/* ARGB */
#define WHITE 0xFFFFFFFF
#define CLEAR 0x00FFFFFF
#define GRAY 0xFF7F7F7F
#define BLACK 0xFF000000
#define RED 0xFF0000FF

void DrawRectangle(int posX, int posY, int width, int height,
                   clay_rgba_t color) {
  clay_vertex_t vertices[] = {
      (clay_vertex_t){
          .pos = {.x = posX, .y = posY, .z = 0},
          .uv = {.x = 0, .y = 0},
          .color = color,
      },
      (clay_vertex_t){
          .pos = {.x = posX + width, .y = posY, .z = 0},
          .uv = {.x = 1, .y = 0},
          .color = color,
      },
      (clay_vertex_t){
          .pos = {.x = posX, .y = posY + height, .z = 0},
          .uv = {.x = 0, .y = 1},
          .color = color,
      },
      (clay_vertex_t){
          .pos = {.x = posX + width, .y = posY + height, .z = 0},
          .uv = {.x = 1, .y = 1},
          .color = color,
      }};

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  clay_vertex_t* submission_pointer = &vertices[0];
  glVertexPointer(3, GL_FLOAT, sizeof(clay_vertex_t), &submission_pointer->pos);
  glTexCoordPointer(2, GL_FLOAT, sizeof(clay_vertex_t),
                    &submission_pointer->uv);
  glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(clay_vertex_t),
                 &submission_pointer->color);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

#define PI 3.14159265
#define SMOOTH_CIRCLE_ERROR_RATE 0.5f
void DrawRoundedRect(int x, int y, int width, int height, float cornerRadius,
                     int segments, clay_rgba_t color) {
  if (cornerRadius >= 1.0f) cornerRadius = 1.0f;

  Rectangle rec = (Rectangle){.x = x, .y = y, .width = width, .height = height};

  // Calculate corner radius
  float radius = (rec.width > rec.height) ? (rec.height * cornerRadius) / 2
                                          : (rec.width * cornerRadius) / 2;
  if (radius <= 0.0f) return;

  // Calculate number of segments to use for the corners
  if (segments < 4) {
    // Calculate the maximum angle between segments based on the error rate
    // (usually 0.5f)
    float th = acosf(2 * powf(1 - SMOOTH_CIRCLE_ERROR_RATE / radius, 2) - 1);
    segments = (int)(ceilf(2 * PI / th) / 4.0f);
    if (segments <= 0) segments = 4;
  }

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
  const float angles[4] = {180.0f, 270.0f, 0.0f, 90.0f};

  // Allocate memory for vertices (adjust size as needed)
  static clay_vertex_t vertices[141];  // Adjust size based on segments
  int vertexIndex = 0;

  for (int corner = 0; corner < 4; corner++) {  // corner 4
    // Generate vertices for each segment of the corner
    for (int i = 0; i < segments; i++) {
      float angle =
          angles[corner] + i * stepLength;  // Convert degrees to radians
      float radians = angle * PI / 180.0f;
      float angleNext =
          angles[corner] + (i + 1) * stepLength;  // Convert degrees to radians
      float radiansNext = angleNext * PI / 180.0f;

      // Center of the corner arc
      float centerX = centers[corner].x;
      float centerY = centers[corner].y;

      // Calculate vertex coordinates
      vertices[vertexIndex++] = (clay_vertex_t){
          .pos = {.x = centerX + radius * cos(radians),
                  .y = centerY + radius * sin(radians),
                  .z = 0},
          .uv = {.x = 0, .y = 0},
          .color = color,
      };
      vertices[vertexIndex++] = (clay_vertex_t){
          .pos = {.x = centerX + radius * cos(radiansNext),
                  .y = centerY + radius * sin(radiansNext),
                  .z = 0},
          .uv = {.x = 0, .y = 0},
          .color = color,
      };
      vertices[vertexIndex++] = (clay_vertex_t){
          .pos = {.x = centerX, .y = centerY, .z = 0},
          .uv = {.x = 0, .y = 0},
          .color = color,
      };
    }
  }

  // Rectangles
  // top
  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[0].x, .y = point[0].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[1].x, .y = point[1].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[8].x, .y = point[8].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[8].x, .y = point[8].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[1].x, .y = point[1].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[9].x, .y = point[9].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };
  // middle
  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[7].x, .y = point[7].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[2].x, .y = point[2].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[6].x, .y = point[6].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[6].x, .y = point[6].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[2].x, .y = point[2].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[3].x, .y = point[3].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };
  // bottom
  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[11].x, .y = point[11].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[10].x, .y = point[10].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[5].x, .y = point[5].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[5].x, .y = point[5].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[10].x, .y = point[10].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[4].x, .y = point[4].y, .z = 0},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  clay_vertex_t* submission_pointer = &vertices[0];
  glVertexPointer(3, GL_FLOAT, sizeof(clay_vertex_t), &submission_pointer->pos);
  glTexCoordPointer(2, GL_FLOAT, sizeof(clay_vertex_t),
                    &submission_pointer->uv);
  glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(clay_vertex_t),
                 &submission_pointer->color);

  glDrawArrays(GL_TRIANGLES, 0, vertexIndex);

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void DrawRing(Clay_Vector2 center, float innerRadius, float outerRadius,
              float startAngle, float endAngle, int segments,
              clay_rgba_t color) {
  float radius = outerRadius;
  // Calculate number of segments to use for the corners
  if (segments < 4) {
    // Calculate the maximum angle between segments based on the error rate
    // (usually 0.5f)
    float th = acosf(2 * powf(1 - SMOOTH_CIRCLE_ERROR_RATE / radius, 2) - 1);
    segments = (int)(ceilf(2 * PI / th) / 4.0f);
    if (segments <= 0) segments = 4;
  }

  float stepLength =
      ((int)fabsf(endAngle - startAngle) % 360) / (float)segments;

  // Allocate memory for vertices (adjust size as needed)
  static clay_vertex_t vertices[1000];  // Adjust size based on segments
  int vertexIndex = 0;

  // Generate vertices for each segment of the corner
  for (int i = 0; i < segments; i++) {
    float angle = startAngle + i * stepLength;  // Convert degrees to radians
    float radians = angle * PI / 180.0f;
    float angleNext =
        startAngle + (i + 1) * stepLength;  // Convert degrees to radians
    float radiansNext = angleNext * PI / 180.0f;

    // Center of the corner arc
    float centerX = center.x;
    float centerY = center.y;

    // Calculate vertex coordinates
    vertices[vertexIndex++] = (clay_vertex_t){
        .pos = {.x = centerX + radius * cos(radians),
                .y = centerY + radius * sin(radians),
                .z = 0},
        .uv = {.x = 0, .y = 0},
        .color = color,
    };
    vertices[vertexIndex++] = (clay_vertex_t){
        .pos = {.x = centerX + radius * cos(radiansNext),
                .y = centerY + radius * sin(radiansNext),
                .z = 0},
        .uv = {.x = 0, .y = 0},
        .color = color,
    };
    vertices[vertexIndex++] = (clay_vertex_t){
        .pos = {.x = centerX, .y = centerY, .z = 0},
        .uv = {.x = 0, .y = 0},
        .color = color,
    };
  }

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  clay_vertex_t* submission_pointer = &vertices[0];
  glVertexPointer(3, GL_FLOAT, sizeof(clay_vertex_t), &submission_pointer->pos);
  glTexCoordPointer(2, GL_FLOAT, sizeof(clay_vertex_t),
                    &submission_pointer->uv);
  glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(clay_vertex_t),
                 &submission_pointer->color);

  glDrawArrays(GL_TRIANGLES, 0, vertexIndex);

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Clay_Renderer_Initialize(int width, int height, const char* title) {
  Clay_Platform_Initialize(width, height, title);

  intraFontInit();
  /* Dreamcast recommended to use INTRAFONT_CACHE_ASCII as an option but not
   * required */
  font = intraFontLoad(PATH_ASSETS "ltn8.pgf", INTRAFONT_CACHE_ASCII);
  if (!font) {
    return;
  }
  intraFontSetStyle(font, 1.f, BLACK, CLEAR, 0.f, INTRAFONT_ALIGN_LEFT);

  _clay_screenWidth = width;
  _clay_screenHeight = height;
}

void Clay_Renderer_Render(Clay_RenderCommandArray renderCommands) {
  Clay_Platform_Render_Start();

  glClear(GL_COLOR_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glOrtho(0, _clay_screenWidth, _clay_screenHeight, 0, -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glEnable(GL_BLEND);

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
        float scaleSize = fontSize / 16.0f;

        Clay_Color color = renderCommand->config.textElementConfig->textColor;
        clay_rgba_t colorStruct = CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(color);
        uint32_t fontColor = 0;
        memcpy(&fontColor, &colorStruct, sizeof(uint32_t));

        // glDepthFunc(GL_LESS);      // The Type Of Depth Test To Do
        // glDisable(GL_DEPTH_TEST);  // Enables Depth Testing
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CW);
        glCullFace(GL_BACK);
        glDisable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);

        const float adjustX = 4.f;
        const float adjustY = renderCommand->boundingBox.height;  // 12.f;

        bool glScissor = glIsEnabled(GL_SCISSOR_TEST);

        intraFontSetStyle(font, scaleSize, fontColor, CLEAR, 0.f,
                          INTRAFONT_ALIGN_LEFT);

        intraFontPrintEx(font, boundingBox.x + adjustX, boundingBox.y + adjustY,
                         text.chars, text.length);

        if (glScissor) {
          glEnable(GL_SCISSOR_TEST);
        }

        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);

        break;
      }
      case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
          //@Todo: Add Image rendering support
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
        const int y = _clay_screenHeight -
                      (int)roundf(boundingBox.y + boundingBox.height);
        const int height = (int)roundf(boundingBox.height);
        glEnable(GL_SCISSOR_TEST);
        glScissor((int)roundf(boundingBox.x), y, (int)roundf(boundingBox.width),
                  height);
        break;
      }
      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
        glDisable(GL_SCISSOR_TEST);
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
                          CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(config->color));
        } else {
          DrawRectangle(boundingBox.x, boundingBox.y, boundingBox.width,
                        boundingBox.height,
                        CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(config->color));
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
              CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(config->left.color));
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
              CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(config->right.color));
        }
        // Top border
        if (config->top.width > 0) {
          DrawRectangle(
              (int)roundf(boundingBox.x + config->cornerRadius.topLeft),
              (int)roundf(boundingBox.y),
              (int)roundf(boundingBox.width - config->cornerRadius.topLeft -
                          config->cornerRadius.topRight),
              (int)config->top.width,
              CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(config->top.color));
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
              CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(config->bottom.color));
        }
        if (config->cornerRadius.topLeft > 0) {
          DrawRing(
              (Clay_Vector2){
                  roundf(boundingBox.x + config->cornerRadius.topLeft),
                  roundf(boundingBox.y + config->cornerRadius.topLeft)},
              roundf(config->cornerRadius.topLeft - config->top.width),
              config->cornerRadius.topLeft, 180, 270, 10,
              CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(config->top.color));
        }
        if (config->cornerRadius.topRight > 0) {
          DrawRing((Clay_Vector2){roundf(boundingBox.x + boundingBox.width -
                                         config->cornerRadius.topRight),
                                  roundf(boundingBox.y +
                                         config->cornerRadius.topRight)},
                   roundf(config->cornerRadius.topRight - config->top.width),
                   config->cornerRadius.topRight, 270, 360, 10,
                   CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(config->top.color));
        }
        if (config->cornerRadius.bottomLeft > 0) {
          DrawRing((Clay_Vector2){roundf(boundingBox.x +
                                         config->cornerRadius.bottomLeft),
                                  roundf(boundingBox.y + boundingBox.height -
                                         config->cornerRadius.bottomLeft)},
                   roundf(config->cornerRadius.bottomLeft - config->top.width),
                   config->cornerRadius.bottomLeft, 90, 180, 10,
                   CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(config->bottom.color));
        }
        if (config->cornerRadius.bottomRight > 0) {
          DrawRing(
              (Clay_Vector2){roundf(boundingBox.x + boundingBox.width -
                                    config->cornerRadius.bottomRight),
                             roundf(boundingBox.y + boundingBox.height -
                                    config->cornerRadius.bottomRight)},
              roundf(config->cornerRadius.bottomRight - config->bottom.width),
              config->cornerRadius.bottomRight, 0.1, 90, 10,
              CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(config->bottom.color));
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
