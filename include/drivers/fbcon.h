#ifndef FBCON_H
#define FBCON_H

#define FBCON_GRID_WIDTH 150
#define FBCON_GRID_HEIGHT 50

#include <kint.h>

typedef struct fbcon_t
{
    uint64_t dev_id;
} fbcon_t;

void fbcon_init();

void fbcon_render();

void fbcon_scroll();

#endif