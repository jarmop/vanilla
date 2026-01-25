#ifndef TYPES_H
#define TYPES_H

// ------------------------- shm buffer wrapper -------------------------

struct shm_buffer {
    void *data;
    int width, height;
    int stride;
    int size;
};

#endif