#include <stdio.h>
#include <string.h>
#include "window.h"
#include "draw.h"

#define text_size 1024

char text[text_size];

recreate_buffer_cb_type recreate_buffer_cb;

static void key_cb(xkb_keysym_t key) {
    int text_len = strlen(text);
    if (key == XKB_KEY_Return) {
        text[text_len] = '\n';
    } else if (key == XKB_KEY_Tab){
        fprintf(stderr, "tab");
        for (int i = 0; i < 2; i++) {
            text[text_len + i] = ' ';
        }
        text_len+=3;
    } else if (key == XKB_KEY_BackSpace) {
        text[text_len - 1] = '\0';
    } else {
        text[text_len] = key;
    }
    text[text_len + 1] = '\0';

    fprintf(stderr, "text: '%s'\n", text);
    recreate_buffer_cb();
}

static void draw_cb(struct shm_buffer *buf) {
    draw(buf, text);
}

int main() {
    FILE *file = fopen("proto/data/exit.c","rb");
    size_t n;
    while ((n = fread(text, sizeof(*text), sizeof(text), file)) > 0) {
        printf("%s\n", text);
    }

    initialize_draw();

    recreate_buffer_cb = initialize_window(&draw_cb, &key_cb);

    run_window();

    return 0;
}