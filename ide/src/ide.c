#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

#include "types.h"
#include "window.h"
#include "draw.h"

char orig_text[] = "0-2345678901234567890\n1-2345678901234567890\n2-2345678901234567890";
struct cursor cursor = {0, 0};
struct text text;

recreate_buffer_cb_type recreate_buffer_cb;

static void initialize_text(char *o_text) {
    int startofline = 0;
    int li = 0;
    int lsi = 0;
    for (int i = 0; o_text[i] != '\0'; i++) {
        if (o_text[i] == '\n' ) {
            text.lines[lsi].length = li;
            lsi++;
            li = 0;
            continue;
        } 
        text.lines[lsi].text[li] = o_text[i];
        li++;
    }
    text.lines[lsi].length = li;
    text.linecount = lsi + 1;

    for (int i = 0; i <= text.linecount; i++) {
        printf("%s\n", text.lines[i].text);
    }
}

static void key_cb(xkb_keysym_t key) {
    if (key == XKB_KEY_Left) {
        cursor.col && cursor.col--;
    } else if (key == XKB_KEY_Right) {
        (cursor.col < text.lines[cursor.row].length) && cursor.col++;
    } else if (key == XKB_KEY_Up) {
        cursor.row && cursor.row--;
    } else if (key == XKB_KEY_Down) {
        (cursor.row < text.linecount - 1) && cursor.row++;
    } else if (key == XKB_KEY_Next) {
        cursor.col = text.lines[text.linecount - 1].length;
        cursor.row = text.linecount - 1;
    } else if (key == XKB_KEY_Prior) {
        cursor.col = 0;
        cursor.row = 0;
    } else if (key == XKB_KEY_Home) {
        cursor.col = 0;
    } else if (key == XKB_KEY_End) {
        cursor.col = text.lines[cursor.row].length;
    } else if (key == XKB_KEY_Return) {
        struct line newline;
        struct line *line = &text.lines[cursor.row];

        // Split the current line into two
        strcpy(newline.text, line->text + cursor.col);
        newline.length = line->length - cursor.col;
        line->text[cursor.col] = '\0';
        line->length = cursor.col;

        // Push the rest of the lines forward
        for (int i = text.linecount; i > cursor.row; i--) {
            text.lines[i + 1] = text.lines[i];
        }

        // Point the next line to the new line
        text.lines[cursor.row + 1] = newline;
        text.linecount++;
        
        cursor.col = 0;
        cursor.row++;
    } else if (key == XKB_KEY_Tab){
        int tab_size = 2;
        char aft[1024];
        struct line *line = &text.lines[cursor.row];
        strcpy(aft, line->text + cursor.col);
        for (int i = 0; i < tab_size; i++) {
            line->text[cursor.col] = ' ';
            cursor.col++;
        }
        strcpy(line->text + cursor.col, aft);
    } else if (key == XKB_KEY_Delete) {
        struct line *currline = &text.lines[cursor.row];
        struct line *nextline = &text.lines[cursor.row + 1];
        if (cursor.col == currline->length) {
            if (cursor.row == text.linecount - 1) {
                return;
            }
            strcpy(currline->text + currline->length, nextline->text);
            currline->length += nextline->length;
            for (int i = cursor.row + 1; i < text.linecount; i++) {
                text.lines[i] = text.lines[i + 1];
            }
            text.linecount--;
        } else {
            char aft[1024];
            struct line *line = &text.lines[cursor.row];
            strcpy(aft, line->text + cursor.col + 1);
            strcpy(line->text + cursor.col, aft);
            line->length--;
        }
        
    } else if (key == XKB_KEY_BackSpace) {
        if (cursor.col == 0) {
            return;
        }
        char aft[1024];
        struct line *line = &text.lines[cursor.row];
        strcpy(aft, line->text + cursor.col);
        strcpy(line->text + cursor.col - 1, aft);
        cursor.col--;
    } else {
        char aft[1024];
        struct line *line = &text.lines[cursor.row];
        strcpy(aft, line->text + cursor.col); 
        line->text[cursor.col] = key;
        line->length++;
        cursor.col++;
        strcpy(line->text + cursor.col, aft);
    }

    recreate_buffer_cb();
}

static void draw_cb(struct shm_buffer *buf) {
    draw(buf, &text, &cursor);
}

int main() {
    // FILE *file = fopen("proto/data/exit.c","rb");
    // size_t n;
    // while ((n = fread(text, sizeof(*text), sizeof(text), file)) > 0) {
    //     printf("%s\n", text);
    // }
    // char foo[5] = {0};

    initialize_text(orig_text);

    initialize_draw();

    recreate_buffer_cb = initialize_window(&draw_cb, &key_cb);

    run_window();

    return 0;
}