#ifndef TYPES_H
#define TYPES_H

#include <stdint.h> 

struct shm_buffer {
    void *data;
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

#endif