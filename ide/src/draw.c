#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "draw.h"
#include "helpers.h"

FT_Library ft;
FT_Face face;

// create the 32-bit XRGB8888 representation of the pixel
static inline uint32_t xrgb(uint8_t r, uint8_t g, uint8_t b) {
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

// Alpha-blend a grayscale coverage value (0..255) as “source alpha”
// over an opaque destination pixel. Foreground color is (fr,fg,fb).
static inline uint32_t blend_coverage_over_xrgb(
    uint32_t dst, uint8_t cov, uint8_t fr, uint8_t fg, uint8_t fb
) {
    if (cov == 0) return dst;
    if (cov == 255) return xrgb(fr, fg, fb);

    // A performant C implementation of: out = b + (f - b) * a
    #define blend(b, f, a) (b + (int)((((int)f - (int)b) * (int)a + 127) / 255))

    uint8_t dr = dst >> 16;
    uint8_t dg = dst >> 8;
    uint8_t db = dst;

    return xrgb(blend(dr, fr, cov), blend(dg, fg, cov), blend(db, fb, cov)); 
}

/**
 * Copy bitmaps from one memory location to another using the BLIT method 
 * (BLock Image Transfer) 
 */
static void draw_glyph(
    struct bitmap *container,
    const FT_Bitmap *glyph,
    int dst_x,
    int dst_y,
    uint8_t fr, uint8_t fg, uint8_t fb
) {
    /**
     * Temporary pointers for the glyph buffer (src) and the destination area 
     * in the container (dst).
     */
    uint8_t *src = glyph->buffer;
    uint32_t *dst = (uint32_t *)container->buffer + dst_y * container->width + dst_x;

    for (
        int row = 0;
        row < glyph->rows;
        row++,
        src += glyph->width,
        dst += container->width
    ) {
        int buf_y = dst_y + row;
        // Prevent overflowing buffer vertically
        if (buf_y < 0 || buf_y >= container->height) {
            continue;
        }

        for (int col = 0; col < glyph->width; col++) {
            int bm_x = dst_x + col;
            // Prevent overflowing buffer horizontally
            if (bm_x < 0 || bm_x >= container->width) {
                continue;
            };
            uint8_t cov = src[col];
            uint32_t old = dst[col];
            dst[col] = blend_coverage_over_xrgb(old, cov, fr, fg, fb);
        }
    }
}

static void draw_text(
    struct bitmap *container,
    int baseline_x,
    int baseline_y,
    struct line *lines,
    int linecount,
    uint8_t fr, uint8_t fg, uint8_t fb
) {
    int pen_x = baseline_x;
    int pen_y = baseline_y;
    int lineheight = (int)(face->size->metrics.height >> 6);

    int line_limit = min(linecount, (container->height - baseline_y / lineheight) + 1);

    for (int i = 0; i < line_limit; i++) {
        char *linetext = lines[i].text;
        for (const unsigned char *p = (const unsigned char*)linetext; *p; p++) {
            unsigned char c = *p;

            // Render glyph bitmap
            if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0) {
                continue; // skip missing glyphs
            }

            FT_GlyphSlot g = face->glyph;

            // Position: bitmap_left is x offset from pen,
            // bitmap_top is y offset above baseline.
            int x = pen_x + g->bitmap_left;
            int y = pen_y - g->bitmap_top;

            if (g->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
                draw_glyph(container, &g->bitmap, x, y, fr, fg, fb);
            } else {
                // For a first pass, ignore other pixel modes.
            }

            pen_x += (int)(g->advance.x >> 6);
        }

        pen_x = baseline_x;
        // crude line advance: use face metrics
        if (lineheight <= 0) lineheight = 16;
        pen_y += lineheight;
    }

}

int initialize_draw(int font_size) {
    // Font path: pass as argv[1], otherwise use a common default on many Linux distros.
    const char *font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
    
    if (FT_Init_FreeType(&ft) != 0) {
        fprintf(stderr, "FT_Init_FreeType failed\n");
        exit(1);
    }

    if (FT_New_Face(ft, font_path, 0, &face) != 0) {
        fprintf(stderr, "FT_New_Face failed (font path: %s)\n", font_path);
        FT_Done_FreeType(ft);
        exit(1);
    }

    // Set pixel size (height in px). Width 0 = auto from height.
    if (FT_Set_Pixel_Sizes(face, 0, font_size) != 0) {
        fprintf(stderr, "FT_Set_Pixel_Sizes failed\n");
    }

    int line_height = (int)(face->size->metrics.height >> 6);

    return line_height;
}


static void fill_solid_xrgb(struct bitmap *bm, uint32_t xrgb) {
    uint32_t *p = (uint32_t*)bm->buffer;
    int n = bm->width * bm->height;
    for (int i = 0; i < n; i++) p[i] = xrgb;
}

static void draw_rectangle(uint32_t *p, int container_width, int x, int y, int width, int height) {
    int row_start = y * container_width;
    int row_end = row_start + container_width * height;
    for (int row=row_start; row<row_end; row+=container_width) {
        int i_start = row + x;
        int i_end = i_start + width;
        for (int i=i_start; i<i_end; i++) {
            p[i] = 0xffffffff; 
        }
    }
}

static void draw_textbox(struct bitmap *bm, struct textbox *textbox, struct cursor *cursor) {
    struct text* text = &textbox->text;
    int text_width = (int)(face->size->metrics.max_advance >> 6);
    int line_height = (int)(face->size->metrics.height >> 6);

    int text_x = text->offset_x + textbox->padding;
    int text_y = text->offset_y + textbox->padding;
    int pen_x = text_x;
    int pen_y = text_y + line_height;

    struct bitmap textbox_bm;
    textbox_bm.width = textbox->width;
    textbox_bm.height = textbox->height;
    textbox_bm.stride = textbox_bm.width * 4;
    textbox_bm.size = textbox_bm.stride * textbox_bm.height;
    textbox_bm.buffer = malloc(textbox_bm.size);

    fill_solid_xrgb(&textbox_bm, 0x222222);
    // fill_solid_xrgb(&textbox_bm, 0xFFFFFF);
    // fill_solid_xrgb(&textbox_bm, 0xDDDDDD);
    // fill_solid_xrgb(&textbox_bm, 0xCCCCCC);
    // fill_solid_xrgb(&textbox_bm, 0xBBBBBB);

    draw_text(
        &textbox_bm,
        pen_x,
        pen_y,
        text->lines,
        text->linecount,
        // 0x22,
        // 0x22,
        // 0x22
        0xBB,
        0xBB,
        0xBB
    );    

    if (textbox->has_focus) {
        int cursor_width = text_width;
        int cursor_height = line_height;
        int cursor_x = text_x + cursor->col * text_width;
        int cursor_y = text_y + cursor->row * line_height;
        if (
            cursor_x >= 0 
            && cursor_x + cursor_width <= textbox_bm.width 
            && cursor_y >= 0 
            && cursor_y + cursor_height <= textbox_bm.height 
        ) {
            draw_rectangle(textbox_bm.buffer, textbox_bm.width,  cursor_x, cursor_y + cursor_height, cursor_width, 1);
            draw_rectangle(textbox_bm.buffer, textbox_bm.width, cursor_x, cursor_y, 1, cursor_height);
        }
    }

    // Copy textbox buffer on the container buffer
    uint32_t *bp = bm->buffer + textbox->x + textbox->y * bm->width;
    uint32_t *tp = textbox_bm.buffer;
    for (int i = 0; i < textbox_bm.height; i++) {
        for (int j = 0; j < textbox_bm.width; j++) {
            bp[j] = tp[j];
        }
        bp += bm->width;
        tp += textbox_bm.width;
    }

    free(textbox_bm.buffer);
}

void draw(struct bitmap *bm, struct textbox *textboxes, int tblen, struct cursor *cursor) {
    u32 border_color = 0x222222;
    // u32 border_color = 0x777777;
    // u32 border_color = 0xCCCCCC;
    // u32 border_color = 0;
    // u32 bg_color = 0x777777;
    // u32 bg_color = 0x999999;
    u32 bg_color = 0xBBBBBB;
    // u32 bg_color = 0x222222;
    // u32 bg_color = 0;

    fill_solid_xrgb(bm, border_color);

    int border_width = 2;
    struct bitmap cbm;
    // cbm.width = bm->width - 2 * border_width;
    cbm.width = 250;
    // cbm.height = bm->height - 2 * border_width;
    cbm.height = 100;
    cbm.stride = cbm.width * 4;
    cbm.size = cbm.stride * cbm.height;
    cbm.buffer = malloc(cbm.size);

    fill_solid_xrgb(&cbm, bg_color);

    uint32_t *bp = bm->buffer + border_width + border_width * bm->width;
    uint32_t *cp = cbm.buffer;
    for (int i = 0; i < cbm.height; i++) {
        for (int j = 0; j < cbm.width; j++) {
            bp[j] = cp[j];
        }
        bp += bm->width;
        cp += cbm.width;
    }

    for (int i = 0; i < tblen; i++) {
        draw_textbox(bm, textboxes + i, cursor);
    }

    free(cbm.buffer);
}