#ifndef CLAY_TYPES_H
#define CLAY_TYPES_H

#include <stdint.h>

#define CUSTOM_CAMERA_PERSPECTIVE (0)
#define CUSTOM_CAMERA_ORTHO (1)

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
  uint8_t b, g, r, a;
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

typedef struct {
  int projection;
  int fovy;
  float near;
  float far;
  clay_vec3_t position;
  clay_vec3_t target;
  clay_vec3_t up;
} Camera;

typedef enum {
  CUSTOM_LAYOUT_ELEMENT_TYPE_NULL,
  CUSTOM_LAYOUT_ELEMENT_TYPE_3D_MODEL,
} CustomElementType;

typedef struct {
  int foo;
} CustomLayoutElement_Null;

typedef struct {
  // Model model;
  float scale;
  clay_vec3_t position;
  // Matrix rotation;
} CustomLayoutElement_3DModel;

typedef struct CustomLayoutElement {
  CustomElementType type;
  union {
    CustomLayoutElement_Null null;
    CustomLayoutElement_3DModel model;
  };
} CustomLayoutElement;

typedef struct {
  clay_vec3_t position;
  clay_vec3_t direction;
} Ray;

#endif  // CLAY_TYPES_H
