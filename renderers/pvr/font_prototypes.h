/*
 * File: font_prototypes.h
 * Project: ui
 * File Created: Thursday, 20th May 2021 12:35:54 am
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once
#ifndef FONT_PROTOTYPES_H
#define FONT_PROTOTYPES_H

/* BMF formatted nice text */
int font_bmf_init(const char *fnt, const char *texture, int is_wide);
void font_bmf_destroy(void);

void font_bmf_begin_draw(void);
void font_bmf_set_scale(float scale);
void font_bmf_set_height_default(void);
void font_bmf_set_height(float height);

void font_bmf_draw(int x, int y, uint32_t color, const char *str);
void font_bmf_draw_main(int x, int y, uint32_t color, const char *str);
void font_bmf_draw_sub(int x, int y, uint32_t color, const char *str);
void font_bmf_draw_sub_wrap(int x, int y, uint32_t color, const char *str, int width);
void font_bmf_draw_auto_size(int x, int y, uint32_t color, const char *str, int width);
void font_bmf_draw_centered(int x, int y, uint32_t color, const char *str);
void font_bmf_draw_centered_auto_size(int x, int y, uint32_t color, const char *str, int width);

void z_set(int newZ);
float font_bmf_get_original_height(void);
float font_bmf_get_current_height(void);
float font_bmf_calculate_length_slice(const char *str, int length);
void font_bmf_draw_slice(int x, int y, uint32_t color, const char *str, int length);

#endif // FONT_PROTOTYPES_H
