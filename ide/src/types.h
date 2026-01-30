#ifndef TYPES_H
#define TYPES_H

struct shm_buffer {
    void *data;
    int width, height;
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
};

struct cursor {
    int col;
    int row;
};

#endif