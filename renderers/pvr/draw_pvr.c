#include "draw_pvr.h"

#include <stdio.h>

#include "pvr_texture.h"

void *draw_load_texture_buffer(const char *filename, void *user, void *buffer) {
  image *img = (image *)user;

  pvr_ptr_t txr;

  if (!(txr = load_pvr_to_buffer(filename, &img->width, &img->height,
                                 &img->format, buffer))) {
    // img->texture = img_empty_boxart.texture;
    // img->width = img_empty_boxart.width;
    // img->height = img_empty_boxart.height;
    // img->format = img_empty_boxart.format;
    // return img;
    printf("failed to load texture!\n");
  }
  img->texture = txr;

  return user;
}
