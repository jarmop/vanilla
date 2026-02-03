#ifndef DRAW_H
#define DRAW_H

#include "types.h"

int initialize_draw(int lineheight);
void draw(struct shm_buffer *buf, struct text *text, struct cursor *cursor);

#endif