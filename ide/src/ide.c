#include <stdio.h>
#include <string.h>
#include "window.h"
#include "draw.h"

char text[128] = "B";

recreate_buffer_cb_type recreate_buffer_cb;

static void key_cb(xkb_keysym_t key) {
    const int text_len = strlen(text);
    fprintf(stderr, "text length: %d\n", text_len);

    if (key == XKB_KEY_Return) {
        text[text_len] = '\n';
    } else if (key == XKB_KEY_BackSpace) {
        // fprintf(stderr, "backspace:\n");
        text[text_len - 1] = '\0';
    } else {
        text[text_len] = key;
    }
    text[text_len + 1] = '\0';

    fprintf(stderr, "text: '%s'\n", text);
    // fprintf(stderr, "text: %d, %d, %d, %d\n", text[0], text[1], text[2], text[3]);
    recreate_buffer_cb();
}

static void draw_cb(struct shm_buffer *buf) {
    draw(buf, text);
}


int main() {
    initialize_draw();

    recreate_buffer_cb = initialize_window(&draw_cb, &key_cb);

    run_window();

    return 0;
}