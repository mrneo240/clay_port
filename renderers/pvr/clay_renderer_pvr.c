#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "clay_platform.h"
#include "clay_renderer.h"
#include "clay_types.h"
#include "font_prototypes.h"
#include "simple_texture_allocator.h"

#define SOFTWARE_SCISSORING (1)

static pvr_poly_hdr_t hdr;

static int _clay_screenHeight = 0;
static int _clay_screenWidth = 0;

#define CLAY_RECTANGLE_TO_OPENGL_LEGACY_RECTANGLE(rectangle)      \
  (Rectangle) {                                                   \
    .x = rectangle.x, .y = rectangle.y, .width = rectangle.width, \
    .height = rectangle.height                                    \
  }
#define CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(color)                  \
  (clay_rgba_t) {                                                 \
    .r = (unsigned char)(color.r), .g = (unsigned char)(color.g), \
    .b = (unsigned char)(color.b), .a = (unsigned char)(color.a)  \
  }

/* ARGB */
#define WHITE (0xFFFFFFFF)
#define CLEAR (0x00FFFFFF)
#define GRAY (0xFF7F7F7F)
#define BLACK (0xFF000000)

#define RED (0xFFFF0000)
#define GREEN (0xFF00FF00)
#define BLUE (0xFF0000FF)

void* fonts[2] = {0, 0};

static int sanitizeFontId(int fontId) {
  if (fontId < 0) {
#if DEBUG
    printf("Cant find font %d , based on 0 min\n", fontId);
#endif
    fontId = 0;
  }
  if (fontId >= (sizeof(fonts) / sizeof(void*))) {
#if DEBUG
    printf("Cant find font %d , based on %d max\n", fontId,
           (sizeof(fonts) / sizeof(void*)));
#endif
    fontId = 0;
  }
  return fontId;
}

static Rectangle current_scissor_region = {0};
static bool scissor_enabled = false;
#if defined(SOFTWARE_SCISSORING)

#define CLIP_MINX (0x1)
#define CLIP_MAXX (0x2)
#define CLIP_MINY (0x4)
#define CLIP_MAXY (0x8)

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                    \
  ((byte) & 0x80 ? '1' : '0'), ((byte) & 0x40 ? '1' : '0'),     \
      ((byte) & 0x20 ? '1' : '0'), ((byte) & 0x10 ? '1' : '0'), \
      ((byte) & 0x08 ? '1' : '0'), ((byte) & 0x04 ? '1' : '0'), \
      ((byte) & 0x02 ? '1' : '0'), ((byte) & 0x01 ? '1' : '0')

static clay_vertex_t _clipped_vertices[150];
void ClipVerticesToScissor(clay_vertex_t* vertices, int num_vertices,
                           int* out_num_vertices) {
  if (!scissor_enabled) {
    *out_num_vertices = num_vertices;
    return;
  }

  const int clip_x_min = current_scissor_region.x;
  const int clip_x_max =
      current_scissor_region.x + current_scissor_region.width;

  const int clip_y_min = current_scissor_region.y;
  const int clip_y_max =
      current_scissor_region.y + current_scissor_region.height;

  bool needed_clipping = false;
  int vertices_out = 0;

  for (int i = 0; i < num_vertices / 3; i++) {
    clay_vertex_t* vertex_0 = &vertices[(i * 3) + 0];
    uint8_t clip_bits[3] = {0, 0, 0};
    for (int vi = 0; vi < 3; vi++) {
      clay_vertex_t* vertex = vertex_0 + vi;
      if (vertex->pos.x < clip_x_min) {
        clip_bits[vi] |= CLIP_MINX;
      }
      if (vertex->pos.x > clip_x_max) {
        clip_bits[vi] |= CLIP_MAXX;
      }

      if (vertex->pos.y < clip_y_min) {
        clip_bits[vi] |= CLIP_MINY;
      }
      if (vertex->pos.y > clip_y_max) {
        clip_bits[vi] |= CLIP_MAXY;
      }
    }

    unsigned short clip_status = 0;

    clip_status = (clip_bits[0] & 0xf) << 8 | (clip_bits[1] & 0xf) << 4 |
                  (clip_bits[2] & 0xf) << 0;

    memcpy(_clipped_vertices + (i * 3) + 0, vertex_0,
           sizeof(clay_vertex_t) * 3);
    vertices_out += 3;

    if (!clip_status) {
      continue;
    }

    needed_clipping = true;

#if 0
    printf("clipping vert bits: " BYTE_TO_BINARY_PATTERN
           " , " BYTE_TO_BINARY_PATTERN " , " BYTE_TO_BINARY_PATTERN " \n",
           BYTE_TO_BINARY((uint8_t)clip_bits[0]),
           BYTE_TO_BINARY((uint8_t)clip_bits[1]),
           BYTE_TO_BINARY((uint8_t)clip_bits[2]));
#endif

    clay_vertex_t* clip_vertex_0 = _clipped_vertices + (i * 3) + 0;
    for (int vi = 0; vi < 3; vi++) {
      clay_vertex_t* vertex = clip_vertex_0 + vi;
      uint8_t vert_clip_bits = clip_bits[vi];
      if (vert_clip_bits & CLIP_MINX) {
        vertex->pos.x = clip_x_min;
      }
      if (vert_clip_bits & CLIP_MAXX) {
        vertex->pos.x = clip_x_max;
      }

      if (vert_clip_bits & CLIP_MINY) {
        vertex->pos.y = clip_y_min;
      }
      if (vert_clip_bits & CLIP_MAXY) {
        vertex->pos.y = clip_y_max;
      }
    }
  }

  if (!needed_clipping) {
    *out_num_vertices = vertices_out;
    return;
  }

  *out_num_vertices = vertices_out;
  memcpy(vertices, _clipped_vertices, sizeof(clay_vertex_t) * vertices_out);
}
void ClipVerticesToScissor_Strip(clay_vertex_t* vertices, int num_vertices) {
  if (!scissor_enabled) {
    return;
  }

  const int clip_x_min = current_scissor_region.x;
  const int clip_x_max =
      current_scissor_region.x + current_scissor_region.width;

  const int clip_y_min = current_scissor_region.y;
  const int clip_y_max =
      current_scissor_region.y + current_scissor_region.height;

  for (int i = 0; i < num_vertices; i++) {
    clay_vertex_t* vertex = &vertices[i];
    uint8_t clip_bits = 0;
    if (vertex->pos.x < clip_x_min) {
      clip_bits |= CLIP_MINX;
    }
    if (vertex->pos.x > clip_x_max) {
      clip_bits |= CLIP_MAXX;
    }

    if (vertex->pos.y < clip_y_min) {
      clip_bits |= CLIP_MINY;
    }
    if (vertex->pos.y > clip_y_max) {
      clip_bits |= CLIP_MAXY;
    }

    if (!clip_bits) {
      continue;
    }

#if 0
    printf(
        "Clipping vert (%.0f, %.0f) => x [%d, %d] y[%d, %d] with "
        "mask " BYTE_TO_BINARY_PATTERN " \n",
        vertex->pos.x, vertex->pos.y, current_scissor_region.x,
        current_scissor_region.x + current_scissor_region.height,
        current_scissor_region.y,
        current_scissor_region.y + current_scissor_region.height,
        BYTE_TO_BINARY((uint8_t)clip_bits));
#endif

    if (clip_bits & CLIP_MINX) {
      vertex->pos.x = clip_x_min;
    }
    if (clip_bits & CLIP_MAXX) {
      vertex->pos.x = clip_x_max;
    }

    if (clip_bits & CLIP_MINY) {
      vertex->pos.y = clip_y_min;
    }
    if (clip_bits & CLIP_MAXY) {
      vertex->pos.y = clip_y_max;
    }
  }
}

#else
void ClipVerticesToScissor(clay_vertex_t* vertices, int num_vertices,
                           int* out_num_vertices) {
  (void)vertices;
  *out_num_vertices = num_vertices;
}
void ClipVerticesToScissor_Strip(clay_vertex_t* vertices, int num_vertices) {
  (void)vertices;
  (void)num_vertices;
}
#endif
static bool PointOutsideRectangle(const Rectangle* rect, int point_x,
                                  int point_y) {
  return (point_x < rect->x || point_x > rect->x + rect->width ||
          point_y < rect->y || point_y > rect->y + rect->height);
}

static int z = 1;
void DrawRectangle(int posX, int posY, int width, int height,
                   clay_rgba_t color) {
  pvr_vertex_t vert;
  uint32_t col = 0;
  memcpy(&col, &color, sizeof(uint32_t));

  pvr_prim(&hdr, sizeof(hdr));
  vert.flags = PVR_CMD_VERTEX;
  vert.x = posX;
  vert.y = posY;
  vert.z = z;
  vert.argb = col;
  vert.oargb = 0;
  pvr_prim(&vert, sizeof(vert));

  vert.flags = PVR_CMD_VERTEX;
  vert.x = posX + width;
  vert.y = posY;
  pvr_prim(&vert, sizeof(vert));

  vert.flags = PVR_CMD_VERTEX;
  vert.x = posX;
  vert.y = posY + height;
  pvr_prim(&vert, sizeof(vert));

  vert.flags = PVR_CMD_VERTEX_EOL;
  vert.x = posX + width;
  vert.y = posY + height;
  pvr_prim(&vert, sizeof(vert));
}

#define M_PI_F ((float)M_PI)
#define SMOOTH_CIRCLE_ERROR_RATE (0.5f)
static void DrawRoundedRect(int x, int y, int width, int height,
                            float cornerRadius, int segments,
                            clay_rgba_t color) {
  segments = 8; /*@Todo: Change this, maybe */

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
    segments = (int)(ceilf(2 * M_PI_F / th) / 4.0f);
    if (segments <= 0) segments = 4;
  }

  float stepLength = 90.0f / (float)segments;

  // Coordinates of the 12 points that define the rounded rect
  const clay_vec2_t point[12] = {
      {(float)rec.x + radius, (float)rec.y},
      {(float)(rec.x + rec.width) - radius, (float)rec.y},
      {(float)rec.x + rec.width, (float)rec.y + radius},  // PO, P1, P2
      {(float)rec.x + rec.width, (float)(rec.y + rec.height) - radius},
      {(float)(rec.x + rec.width) - radius,
       (float)rec.y + rec.height},  // P3, P4
      {(float)rec.x + radius, (float)rec.y + rec.height},
      {(float)rec.x, (float)(rec.y + rec.height) - radius},
      {(float)rec.x, (float)rec.y + radius},  // P5, P6, P7
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
      float radians = angle * M_PI_F / 180.0f;
      float angleNext =
          angles[corner] + (i + 1) * stepLength;  // Convert degrees to radians
      float radiansNext = angleNext * M_PI_F / 180.0f;

      // Center of the corner arc
      float centerX = centers[corner].x;
      float centerY = centers[corner].y;

      // Calculate vertex coordinates
      vertices[vertexIndex++] = (clay_vertex_t){
          .pos = {.x = centerX + radius * cos(radians),
                  .y = centerY + radius * sin(radians),
                  .z = z},
          .uv = {.x = 0, .y = 0},
          .color = color,
      };
      vertices[vertexIndex++] = (clay_vertex_t){
          .pos = {.x = centerX + radius * cos(radiansNext),
                  .y = centerY + radius * sin(radiansNext),
                  .z = z},
          .uv = {.x = 0, .y = 0},
          .color = color,
      };
      vertices[vertexIndex++] = (clay_vertex_t){
          .pos = {.x = centerX, .y = centerY, .z = z},
          .uv = {.x = 0, .y = 0},
          .color = color,
      };
    }
  }

  // Rectangles
  // top
  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[0].x, .y = point[0].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[1].x, .y = point[1].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[8].x, .y = point[8].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[8].x, .y = point[8].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[1].x, .y = point[1].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[9].x, .y = point[9].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };
  // middle
  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[7].x, .y = point[7].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[2].x, .y = point[2].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[6].x, .y = point[6].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[6].x, .y = point[6].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[2].x, .y = point[2].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[3].x, .y = point[3].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };
  // bottom
  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[11].x, .y = point[11].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[10].x, .y = point[10].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[5].x, .y = point[5].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[5].x, .y = point[5].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[10].x, .y = point[10].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  vertices[vertexIndex++] = (clay_vertex_t){
      .pos = {.x = point[4].x, .y = point[4].y, .z = z},
      .uv = {.x = 0, .y = 0},
      .color = color,
  };

  int numClippedVertices = 0;
  ClipVerticesToScissor(vertices, vertexIndex, &numClippedVertices);

  pvr_vertex_t vert;
  uint32_t col = 0;
  memcpy(&col, &color, sizeof(uint32_t));
  pvr_prim(&hdr, sizeof(hdr));
  vert.z = z;
  vert.argb = col;
  vert.oargb = 0;

  for (int idx = 0; idx < numClippedVertices / 3; idx++) {
    vert.flags = PVR_CMD_VERTEX;
    vert.x = vertices[idx * 3 + 0].pos.x;
    vert.y = vertices[idx * 3 + 0].pos.y;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX;
    vert.x = vertices[idx * 3 + 1].pos.x;
    vert.y = vertices[idx * 3 + 1].pos.y;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = vertices[idx * 3 + 2].pos.x;
    vert.y = vertices[idx * 3 + 2].pos.y;
    pvr_prim(&vert, sizeof(vert));
  }
}

static void DrawRing(Clay_Vector2 center, float innerRadius, float outerRadius,
                     float startAngle, float endAngle, int segments,
                     clay_rgba_t color) {
  segments = 8; /*@Todo: Change this, maybe */

  float radius = outerRadius;
  // Calculate number of segments to use for the corners
  if (segments < 4) {
    // Calculate the maximum angle between segments based on the error rate
    // (usually 0.5f)
    float th = acosf(2 * powf(1 - SMOOTH_CIRCLE_ERROR_RATE / radius, 2) - 1);
    segments = (int)(ceilf(2 * M_PI_F / th) / 4.0f);
    if (segments <= 0) segments = 4;
  }

  float stepLength =
      ((int)fabsf(endAngle - startAngle) % 360) / (float)segments;

  // Allocate memory for vertices (adjust size as needed)
  static clay_vertex_t vertices[24];  // Adjust size based on segments
  int vertexIndex = 0;

  // Generate vertices for each segment of the corner
  for (int i = 0; i < segments; i++) {
    float angle = startAngle + i * stepLength;  // Convert degrees to radians
    float radians = angle * M_PI_F / 180.0f;
    float angleNext =
        startAngle + (i + 1) * stepLength;  // Convert degrees to radians
    float radiansNext = angleNext * M_PI_F / 180.0f;

    // Center of the corner arc
    float centerX = center.x;
    float centerY = center.y;

    // Calculate vertex coordinates
    vertices[vertexIndex++] = (clay_vertex_t){
        .pos = {.x = centerX + radius * cos(radians),
                .y = centerY + radius * sin(radians),
                .z = z},
        .uv = {.x = 0, .y = 0},
        .color = color,
    };
    vertices[vertexIndex++] = (clay_vertex_t){
        .pos = {.x = centerX + radius * cos(radiansNext),
                .y = centerY + radius * sin(radiansNext),
                .z = z},
        .uv = {.x = 0, .y = 0},
        .color = color,
    };
    vertices[vertexIndex++] = (clay_vertex_t){
        .pos = {.x = centerX, .y = centerY, .z = z},
        .uv = {.x = 0, .y = 0},
        .color = color,
    };
  }

  int numClippedVertices = 0;
  ClipVerticesToScissor(vertices, vertexIndex, &numClippedVertices);

  pvr_vertex_t vert;
  uint32_t col = 0;
  memcpy(&col, &color, sizeof(uint32_t));
  pvr_prim(&hdr, sizeof(hdr));
  vert.z = z;
  vert.argb = col;
  vert.oargb = 0;

  for (int idx = 0; idx < numClippedVertices / 3; idx++) {
    vert.flags = PVR_CMD_VERTEX;
    vert.x = vertices[idx * 3 + 0].pos.x;
    vert.y = vertices[idx * 3 + 0].pos.y;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX;
    vert.x = vertices[idx * 3 + 1].pos.x;
    vert.y = vertices[idx * 3 + 1].pos.y;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = vertices[idx * 3 + 2].pos.x;
    vert.y = vertices[idx * 3 + 2].pos.y;
    pvr_prim(&vert, sizeof(vert));
  }
}

static intptr_t pvr_scratch_buf;

void Clay_Renderer_Initialize(int width, int height, const char* title) {
  if (width > 640) {
    width = 640;
  }
  if (height > 480) {
    height = 480;
  }

  Clay_Platform_Initialize(width, height, title);

  pvr_scratch_buf = (intptr_t)pvr_mem_malloc(TEXMAN_BUFFER_SIZE);
  texman_reset(pvr_scratch_buf, TEXMAN_BUFFER_SIZE);
  texman_clear();

  font_bmf_init("FONT/BASILEA.FNT", "FONT/BASILEA_W.PVR", 0);

  printf("Texture scratch free: %d/%d KB (%d/%d bytes)\n",
         texman_get_space_available() / 1024, (1024 * 1024) / 1024,
         texman_get_space_available(), (1024 * 1024));

  _clay_screenWidth = width;
  _clay_screenHeight = height;

  pvr_poly_cxt_t cxt;
  pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
  cxt.gen.shading = PVR_SHADE_GOURAUD;  // PVR_SHADE_FLAT
  pvr_poly_compile(&hdr, &cxt);
}

static void stats(void) {
#if (DEBUG)
  pvr_stats_t stats;

  pvr_get_stats(&stats);
  dbglog(DBG_DEBUG, "3D Stats: %d frames, frame rate ~%f fps\n",
         stats.vbl_count, (double)stats.frame_rate);
#endif
}

void Clay_Renderer_Render(Clay_RenderCommandArray renderCommands) {
  Clay_Platform_Render_Start();

  pvr_scene_begin();
  // pvr_list_begin(PVR_LIST_OP_POLY);
  //  pvr_list_finish();
  pvr_list_begin(PVR_LIST_TR_POLY);

  bool isScissorActive = false;
  z = 1;

  // vertexOffset = 0;

  for (int j = 0; j < renderCommands.length; j++) {
    z++;
    Clay_RenderCommand* renderCommand =
        Clay_RenderCommandArray_Get(&renderCommands, j);
    Clay_BoundingBox boundingBox = renderCommand->boundingBox;
    switch (renderCommand->commandType) {
      case CLAY_RENDER_COMMAND_TYPE_TEXT: {
        Clay_String text = renderCommand->text;
        // Font fontToUse =
        //   Raylib_fonts[renderCommand->config.textElementConfig->fontId].font;

        float fontSize = renderCommand->config.textElementConfig->fontSize;
        // const float adjustX = 4.f;
        // const float adjustY = renderCommand->boundingBox.height;  // 12.f;
        // int currentFontId =
        //    sanitizeFontId(renderCommand->config.textElementConfig->fontId);
        // intraFont* currentFont = fonts[currentFontId];

#if defined(SOFTWARE_SCISSORING)
        if (scissor_enabled &&
            PointOutsideRectangle(&current_scissor_region, boundingBox.x,
                                  boundingBox.y)) {
          break;
        }
#endif

        Clay_Color color = renderCommand->config.textElementConfig->textColor;
        clay_rgba_t colorStruct = CLAY_COLOR_TO_OPENGL_LEGACY_COLOR(color);
        uint32_t fontColor = 0;
        memcpy(&fontColor, &colorStruct, sizeof(uint32_t));

        font_bmf_begin_draw();
        font_bmf_set_height(fontSize);
        z_set(z);
        font_bmf_draw_slice(boundingBox.x, boundingBox.y, fontColor, text.chars,
                            text.length);

#if !defined(SOFTWARE_SCISSORING)
        if (scissor_enabled) {
          // glEnable(GL_SCISSOR_TEST);
        }
#endif

        // glDisable(GL_TEXTURE_2D);
        // glEnable(GL_BLEND);

        break;
      }
      case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
        //@Todo: Add Image rendering support
        /*
        Texture2D imageTexture = *(Texture2D
       *)renderCommand->config.imageElementConfig->imageData;
       DrawTextureEx( imageTexture, (Vector2){boundingBox.x,
       boundingBox.y}, 0, boundingBox.width / (float)imageTexture.width,
        WHITE); //
//
//
        */
        break;
      }
      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
        scissor_enabled = true;
#if defined(SOFTWARE_SCISSORING)
        current_scissor_region = (Rectangle){
            .x = (int)roundf(boundingBox.x),
            .y = (int)roundf(boundingBox.y),
            .width = (int)roundf(boundingBox.width),
            .height = (int)roundf(boundingBox.height),
        };
#else
        const int y = _clay_screenHeight -
                      (int)roundf(boundingBox.y + boundingBox.height);
        const int height = (int)roundf(boundingBox.height);

        // glEnable(GL_SCISSOR_TEST);
        // glScissor((int)roundf(boundingBox.x), y,
        // (int)roundf(boundingBox.width),
        //           height);
#endif
        break;
      }
      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
        scissor_enabled = false;
#if defined(SOFTWARE_SCISSORING)
#else
        // glDisable(GL_SCISSOR_TEST);
#endif
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
      case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
        break;
      default: {
        printf("Error: unhandled render command.");
#ifdef CLAY_OVERFLOW_TRAP
        raise(SIGTRAP);
#endif
        exit(1);
      }
    }
  }

  pvr_list_finish();
  pvr_scene_finish();
  stats();
  Clay_Platform_Render_End();
}

Clay_Dimensions Renderer_MeasureText(Clay_StringSlice text,
                                     Clay_TextElementConfig* config,
                                     uintptr_t userData) {
  Clay_Dimensions textSize = {0};
  // Measure string size for Font

  font_bmf_set_height((float)config->fontSize);
  float width = font_bmf_calculate_length_slice(text.chars, text.length);
  float height = font_bmf_get_current_height();

  textSize.width = width;
  textSize.height = height;
  return textSize;
}

void Clay_Platform_Shutdown();
void Clay_Renderer_Shutdown() {
  font_bmf_destroy();

  Clay_Platform_Shutdown();
}
