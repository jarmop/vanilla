#include <stdio.h>
#include <string.h>
#include "window.h"
#include "draw.h"

#define text_size 1024

// char text[text_size];
char text[text_size] = "0-2345678901234567890\n1-2345678901234567890\n2-2345678901234567890";
int cursor_pos[] = {0, 0};

recreate_buffer_cb_type recreate_buffer_cb;

static void key_cb(xkb_keysym_t key) {
    int text_len = strlen(text);
    int cursor_col = cursor_pos[0];
    int cursor_line = cursor_pos[1];

    int linecount = 0;
    int linelengths[1024] = {0};
    int linelength = 0;
    for (int i = 0; i < text_len; i++) {
        if (text[i] == '\n') {
            linelengths[linecount] = linelength;
            linecount++;
            linelength = 0;
        } else {
            linelength++;
        }
    }
    linelengths[linecount] = linelength;

    int ci = 0;
    for (int l = 0; l < cursor_line; l++) {
        // +1 = add newline character
        ci += linelengths[l] + 1;
    }
    ci += cursor_col;

    // fprintf(stderr, "linelengths: %d --> {%d, %d, %d, %d}\n", linecount, linelengths[0], linelengths[1], linelengths[2], linelengths[3]);
    // fprintf(stderr, "cursor_col: %d\n", cursor_col);
    // fprintf(stderr, "linelengths[cursor_line]: %d\n", linelengths[cursor_line]);
    // fprintf(stderr, "ci: %d\n", ci);
    if (key == XKB_KEY_Left) {
        cursor_pos[0] && cursor_pos[0]--;
    } else if (key == XKB_KEY_Right) {
        (cursor_col != linelengths[cursor_line]) && cursor_pos[0]++;
    } else if (key == XKB_KEY_Up) {
        cursor_pos[1] && cursor_pos[1]--;
    } else if (key == XKB_KEY_Down) {
        (cursor_line < linecount) && cursor_pos[1]++;
    } else if (key == XKB_KEY_Next) {
        cursor_pos[0] = linelengths[linecount];
        cursor_pos[1] = linecount;
    } else if (key == XKB_KEY_Prior) {
        cursor_pos[0] = 0;
        cursor_pos[1] = 0;
    } else if (key == XKB_KEY_Home) {
        cursor_pos[0] = 0;
    } else if (key == XKB_KEY_End) {
        cursor_pos[0] = linelengths[cursor_line];
    } else if (key == XKB_KEY_Return) {
        // backup the text after cursor
        char aft[1024];
        strcpy(aft, text + ci);
        text[ci] = '\n';
        ci++;
        cursor_pos[0] = 0;
        cursor_pos[1]++;
        // add the backupped text after the new character
        strcpy(text + ci, aft);
    } else if (key == XKB_KEY_Tab){
        int tab_size = 2;
        char aft[1024];
        strcpy(aft, text + ci);
        for (int i = 0; i < tab_size; i++) {
            text[ci] = ' ';
            ci++;
            cursor_pos[0]++;
        }
        strcpy(text + ci, aft);
    } else if (key == XKB_KEY_Delete) {
        char aft[1024];
        strcpy(aft, text + ci + 1);
        strcpy(text + ci, aft);
    } else if (key == XKB_KEY_BackSpace) {
        if (cursor_pos[0] == 0) {
            return;
        }
        char aft[1024];
        strcpy(aft, text + ci);
        strcpy(text + ci - 1, aft);
        cursor_pos[0]--;
    } else {
        char aft[1024];
        strcpy(aft, text + ci); 
        text[ci] = key;
        ci++;
        cursor_pos[0]++;
        strcpy(text + ci, aft);
    }

    // fprintf(stderr, "text: '%s'\n", text);
    recreate_buffer_cb();
}

static void draw_cb(struct shm_buffer *buf) {
    draw(buf, text, cursor_pos);
}

int main() {
    // FILE *file = fopen("proto/data/exit.c","rb");
    // size_t n;
    // while ((n = fread(text, sizeof(*text), sizeof(text), file)) > 0) {
    //     printf("%s\n", text);
    // }

    initialize_draw();

    recreate_buffer_cb = initialize_window(&draw_cb, &key_cb);

    run_window();

    return 0;
}