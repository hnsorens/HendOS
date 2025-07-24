/**
 * @file fbcon.h
 * @brief Framebuffer Console Driver Interface
 *
 * Declares framebuffer console structures and function prototypes for text rendering and scrolling.
 */
#ifndef FBCON_H
#define FBCON_H

#define FBCON_GRID_WIDTH 150
#define FBCON_GRID_HEIGHT 50

#include <kint.h>

typedef struct file_descriptor_t file_descriptor_t;

typedef struct fbcon_t
{
    file_descriptor_t* fbcon;
} fbcon_t;

void fbcon_init();

size_t fbcon_render(uint64_t open_file, uint64_t character, uint64_t position);

size_t fbcon_scroll(uint64_t open_file, uint64_t amount, uint64_t _unused);

#endif