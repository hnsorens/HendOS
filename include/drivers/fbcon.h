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

void fbcon_render();

void fbcon_scroll();

#endif