#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define u8 uint8_t
#define u32 uint32_t
#define u64 uint64_t

struct bitmap {
    uint32_t *buffer;
    int width;
    int height;
    int stride;
    int size;
};

struct line {
    char text[1024];
    int length;
};

struct text {
    struct line lines[1024];
    int linecount;
    int lineheight;
    int width;
    int offset_x;
    int offset_y;
};


struct cursor {
    int col;
    int row;
};

struct scroll {
    uint32_t direction;
    uint32_t distance;
};

struct textbox {
    struct text text;
    struct cursor cursor;
    int has_focus;
    int x;
    int y;
    int width;
    int height;
    int padding;
};

#endif