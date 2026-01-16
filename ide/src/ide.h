#ifndef IDE_H
#define IDE_H

#include "xdg-shell-client-protocol.h"

// ------------------------- shm buffer wrapper -------------------------

struct shm_buffer {
    struct wl_buffer *buffer;
    void *data;
    int width, height;
    int stride;
    int size;
};

#endif