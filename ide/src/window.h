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

recreate_buffer_cb_type initialize_window();
int run_window();

#endif