export const code = `
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input-event-codes.h>
#include <time.h>
#include <sys/time.h>

#include "types.h"
#include "window.h"
#include "draw.h"
#include "helpers.h"
#include "compiler.h"

int font_size = 14;
struct cursor cursor = {0, 0};
struct textbox textboxes[2] = {0};
int tblen = 0;

recreate_buffer_cb_type recreate_buffer_cb;

static void new_textbox(int x, int y, int width, int height, char *o_text) {
    int startofline = 0;
    int li = 0;
    int lsi = 0;
    struct textbox textbox = {0};
    textbox.x = x;
    textbox.y = y;
    textbox.width = width;
    textbox.height = height;
    textbox.padding = 4;
    struct text *text = &textbox.text;
    text->width = 0;
    for (int i = 0; o_text[i] != '\0'; i++) {
        if (o_text[i] == '\n' ) {
            text->lines[lsi].length = li;
            if (li > text->width) {
                text->width = li;
            }
            lsi++;
            li = 0;
            continue;
        } 
        text->lines[lsi].text[li] = o_text[i];
        li++;
    }
    text->lines[lsi].length = li;
    text->linecount = lsi + 1;

    textboxes[tblen] = textbox;
    tblen++;
}

static void key_cb(xkb_keysym_t key) {
    if (!textboxes[0].has_focus) {
        return;
    }
    struct text *text = &textboxes[0].text;

    if (key == XKB_KEY_Left) {
        cursor.col && cursor.col--;
    } else if (key == XKB_KEY_Right) {
        (cursor.col < text->lines[cursor.row].length) && cursor.col++;
    } else if (key == XKB_KEY_Up) {
        if (cursor.row == 0) { return; }
        cursor.row--;
        cursor.col = min(cursor.col, text->lines[cursor.row].length);
    } else if (key == XKB_KEY_Down) {
        if (cursor.row == text->linecount - 1) { return; }
        cursor.row++;
        cursor.col = min(cursor.col, text->lines[cursor.row].length);
    } else if (key == XKB_KEY_Next) {
        cursor.col = text->lines[text->linecount - 1].length;
        cursor.row = text->linecount - 1;
    } else if (key == XKB_KEY_Prior) {
        cursor.col = 0;
        cursor.row = 0;
    } else if (key == XKB_KEY_Home) {
        cursor.col = 0;
    } else if (key == XKB_KEY_End) {
        cursor.col = text->lines[cursor.row].length;
    } else if (key == XKB_KEY_Return) {
        struct line newline;
        struct line *line = &text->lines[cursor.row];

        // Split the current line into two
        strcpy(newline.text, line->text + cursor.col);
        newline.length = line->length - cursor.col;
        for (int i = cursor.col; i < line->length; i++) {
            line->text[i] = '\0';
        }
        line->length = cursor.col;

        // Push the rest of the lines forward
        for (int i = text->linecount; i > cursor.row; i--) {
            text->lines[i + 1] = text->lines[i];
        }

        // Point the next line to the new line
        text->lines[cursor.row + 1] = newline;
        text->linecount++;
        
        cursor.col = 0;
        cursor.row++;
    } else if (key == XKB_KEY_Tab){
        int tab_size = 2;
        struct line *line = &text->lines[cursor.row];
        // move the rest of the line forward to make room for the tab
        line->text[line->length + tab_size] = '\0';
        for (int i = line->length - 1; i >= cursor.col; i--) {
            line->text[i + tab_size] = line->text[i];
        }
        // fill the tab with spaces
        for (int i = 0; i < tab_size; i++) {
            line->text[cursor.col] = ' ';
            cursor.col++;
        }
        line->length += tab_size;
    } else if (key == XKB_KEY_Delete) {
        struct line *currline = &text->lines[cursor.row];
        struct line *nextline = &text->lines[cursor.row + 1];
        if (cursor.col == currline->length) {
            if (cursor.row == text->linecount - 1) {
                return;
            }
            strcpy(currline->text + currline->length, nextline->text);
            currline->length += nextline->length;
            for (int i = cursor.row + 1; i < text->linecount; i++) {
                text->lines[i] = text->lines[i + 1];
            }
            text->linecount--;
        } else {
            char aft[1024];
            struct line *line = &text->lines[cursor.row];
            strcpy(aft, line->text + cursor.col + 1);
            strcpy(line->text + cursor.col, aft);
            line->length--;
        }        
    } else if (key == XKB_KEY_BackSpace) {
        if (cursor.col == 0) {
            if (cursor.row == 0) {
                return;
            }
            struct line *currline = &text->lines[cursor.row];
            struct line *previousline = &text->lines[cursor.row - 1];
            strcpy(previousline->text + previousline->length, currline->text);
            cursor.col = previousline->length;
            cursor.row--;
            previousline->length += currline->length;                
            for (int i = cursor.row + 1; i < text->linecount; i++) {
                text->lines[i] = text->lines[i + 1];
            }
            text->linecount--;
        } else {
            char aft[1024];
            struct line *line = &text->lines[cursor.row];
            strcpy(aft, line->text + cursor.col);
            strcpy(line->text + cursor.col - 1, aft);
            cursor.col--;
        }
    } else {
        char aft[1024];
        struct line *line = &text->lines[cursor.row];
        strcpy(aft, line->text + cursor.col); 
        line->text[cursor.col] = key;
        line->length++;
        cursor.col++;
        strcpy(line->text + cursor.col, aft);
    }

    recreate_buffer_cb();
}

struct timespec t1;
struct timespec t2;
uint32_t tms1 = 0;
int rendered_offset_y = 0;
int rendered_offset_x = 0;

static void mouse_cb(uint32_t mouse_event, uint32_t x, uint32_t y, const struct scroll *scroll) {
    int changed = 0;
    int xstride = 5;
    int ystride = 5;
    struct text *text = &textboxes[1].text;

    switch (mouse_event) {
    case BTN_LEFT:
        // fprintf(stderr, "BTN_LEFT\n");
        compile(text->lines[0].text);
        break;
    case BTN_RIGHT:
        // fprintf(stderr, "BTN_RIGHT\n");
        break;
    case REL_WHEEL:
        int min_x = min(0, textboxes[0].width - text->width * 8 - 30);
        int max_x = 0;
        int min_y = (1 - text->linecount) * text->lineheight;
        int max_y = 0;
        text->offset_x -= x * xstride;
        text->offset_y -= y * ystride;

        text->offset_x = min(max(text->offset_x, min_x), max_x);
        text->offset_y = min(max(text->offset_y, min_y), max_y);

        if ((text->offset_y != rendered_offset_y) || (text->offset_x != rendered_offset_x)) {
            changed = 1;
        }

        break;
    default:
        fprintf(stderr, "unrecognized mouse event: %d\n", mouse_event);
        break;
    }

    if (changed) {
        // Throttled drawing. Recreate buffer only if over 30 ms has passed since the last time. 
        int mindiff = 30;
        clock_gettime(CLOCK_MONOTONIC, &t2);
        uint32_t tms2 = t2.tv_sec * 1000 + t2.tv_nsec / 1000000;
        // fprintf(stderr, "timestamp: %u\n", tms2 - tms1);
        if (tms2 - tms1 > mindiff) {
            // fprintf(stderr, "recreate_buffer_cb\n");
            recreate_buffer_cb();
            // draw(&fake_buffer, &text, &cursor);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            tms1 = t1.tv_sec * 1000 + t1.tv_nsec / 1000000;
            rendered_offset_y = text->offset_y;
            rendered_offset_x = text->offset_x;
        }
    }
}

static void draw_cb(struct bitmap *bm) {
    draw(bm, textboxes, tblen, &cursor);
}

int main() {
    // char input_text[1024] = {0};
    // for (int i = 0; i < 8; i+=2) {
    //     input_text[i] = 'O';
    //     input_text[i+1] = '\n';
    // }
    // char input_text[] = "0-2345678901234567890\n1-2345678901234567890\n2-2345678901234567890";
    // char input_text[] = "-";

    // char input_text1[64*1024] = {0};
    // // FILE *file = fopen("proto/data/exit.c","r");
    // FILE *file = fopen("src/ide.c","r");
    // size_t n;
    // while ((n = fread(input_text1, sizeof(*input_text1), sizeof(input_text1), file)) > 0) {
    //     // printf("%s\n", input_text);
    // }

    char input_text1[] = "Print";
    new_textbox(10, 10, 50, 32, input_text1);

    char input_text2[] = "Hello, World!";
    new_textbox(62, 10, 150, 32, input_text2);

    textboxes[0].text.lineheight = initialize_draw(font_size);

    recreate_buffer_cb = initialize_window(&draw_cb, &key_cb, &mouse_cb);

    run_window();

    return 0;
}
`;