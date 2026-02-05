#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "draw.h"

// static void fill_solid_xrgb(struct shm_buffer *buf, uint32_t xrgb) {
//     uint32_t *p = (uint32_t*)buf->data;
//     int n = buf->width * buf->height;
//     for (int i = 0; i < n; i++) p[i] = xrgb;
// }

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
    struct bitmap *buf,
    FT_Face face,
    int baseline_x,
    int baseline_y,
    struct line *lines,
    int linecount,
    uint8_t fr, uint8_t fg, uint8_t fb
) {
    int pen_x = baseline_x;
    int pen_y = baseline_y;

    for (int i = 0; i < linecount; i++) {
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
                draw_glyph(buf, &g->bitmap, x, y, fr, fg, fb);
            } else {
                // For a first pass, ignore other pixel modes.
            }

            pen_x += (int)(g->advance.x >> 6);
        }
        pen_x = baseline_x;
        // crude line advance: use face metrics
        int lineheight = (int)(face->size->metrics.height >> 6);
        if (lineheight <= 0) lineheight = 16;
        pen_y += lineheight;
        continue;
    }

}

FT_Library ft;
FT_Face face;

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

void draw_rectangle(uint32_t *p, int container_width, int x, int y, int width, int height) {
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

void draw(struct bitmap *bm, struct text *text, struct cursor *cursor) {
    int text_width = (int)(face->size->metrics.max_advance >> 6);
    int line_height = (int)(face->size->metrics.height >> 6);

    int text_x = text->offset_x;
    int text_y = text->offset_y;
    int pen_x = text_x;
    int pen_y = text_y + line_height;

    draw_text(
        bm,
        face,
        pen_x, 
        pen_y,
        text->lines,
        text->linecount,
        0xBB, 
        0xBB, 
        0xBB
    );

    int cursor_width = text_width;
    int cursor_height = line_height;
    int cursor_x = text_x + cursor->col * text_width;
    int cursor_y = text_y + cursor->row * line_height;
    if (
        cursor_x >= 0 
        && cursor_x + cursor_width <= bm->width 
        && cursor_y >= 0 
        && cursor_y + cursor_height <= bm->height 
    ) {
        draw_rectangle(bm->buffer, bm->width,  cursor_x, cursor_y + cursor_height, cursor_width, 1);
        draw_rectangle(bm->buffer, bm->width, cursor_x, cursor_y, 1, cursor_height);
    }

}