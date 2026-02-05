#ifndef DRAW_H
#define DRAW_H

#include "types.h"

int initialize_draw(int lineheight);
void draw(struct bitmap *bm, struct textbox *textboxes, int tblen, struct cursor *cursor);

#endif