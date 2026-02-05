#ifndef WINDOW_H
#define WINDOW_H

#include <xkbcommon/xkbcommon.h>


// const uint32_t key_is_not_pressed = 0;
// const uint32_t key_is_pressed = 1;
// const uint32_t key_was_repeated = 2;

// #define key_is_not_pressed 0
// #define key_is_pressed 1
// #define key_was_repeated 2

// #define BACKSPACE XKB_KEY_BackSpace

typedef void (*recreate_buffer_cb_type)(void);

recreate_buffer_cb_type initialize_window(
    void (*draw_cb_arg)(struct bitmap *buf),
    void (*key_cb_arg)(xkb_keysym_t key),
    void (*mouse_cb_arg)(uint32_t mouse_event, uint32_t x, uint32_t y, const struct scroll *scroll)
);
int run_window();

#endif