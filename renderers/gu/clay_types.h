#ifndef CLAY_TYPES_H
#define CLAY_TYPES_H

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
} __attribute__((packed)) clay_vertex_t ;

typedef struct {
  clay_rgba_t color;
  clay_vec3_t pos;
} __attribute__((packed)) clay_color_vertex_t;

typedef struct {
  uint16_t color;
  clay_vec3_16_t pos;
} __attribute__((packed)) clay_color_vertex_16_t ;

typedef struct {
  clay_vertex_t vertices[3];
} clay_tris_t;

#endif  // CLAY_TYPES_H
