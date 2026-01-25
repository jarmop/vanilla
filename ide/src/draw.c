#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "draw.h"

static void fill_solid_xrgb(struct shm_buffer *buf, uint32_t xrgb) {
    uint32_t *p = (uint32_t*)buf->data;
    int n = buf->width * buf->height;
    for (int i = 0; i < n; i++) p[i] = xrgb;
}

// create the 32-bit XRGB8888 representation of the pixel
static inline uint32_t pack_xrgb(uint8_t r, uint8_t g, uint8_t b) {
    return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

// Alpha-blend a grayscale coverage value (0..255) as “source alpha”
// over an opaque destination pixel. Foreground color is (fr,fg,fb).
static inline uint32_t blend_coverage_over_xrgb(
    uint32_t dst, uint8_t cov, uint8_t fr, uint8_t fg, uint8_t fb
) {
    if (cov == 0) return dst;
    if (cov == 255) return pack_xrgb(fr, fg, fb);

    uint8_t dr = (dst >> 16) & 0xFF;
    uint8_t dg = (dst >> 8)  & 0xFF;
    uint8_t db = (dst)       & 0xFF;

    // out = dst + (fg - dst) * a
    // use integer math with rounding
    uint32_t a = cov;

    uint8_t or_ = (uint8_t)(dr + (int)((((int)fr - (int)dr) * (int)a + 127) / 255));
    uint8_t og_ = (uint8_t)(dg + (int)((((int)fg - (int)dg) * (int)a + 127) / 255));
    uint8_t ob_ = (uint8_t)(db + (int)((((int)fb - (int)db) * (int)a + 127) / 255));

    return pack_xrgb(or_, og_, ob_);
}

/**
 * Copy bitmaps from one memory location to another using the BLIT method 
 * (BLock Image Transfer) 
 */
static void blit_ft_bitmap_xrgb(
    struct shm_buffer *buf,
    const FT_Bitmap *bm,
    int dst_x,
    int dst_y,
    uint8_t fr, uint8_t fg, uint8_t fb
) {
    // We assume bm->pixel_mode == FT_PIXEL_MODE_GRAY and bm->num_grays == 256
    for (int row = 0; row < (int)bm->rows; row++) {
        int y = dst_y + row;
        if (y < 0 || y >= buf->height) continue;

        const uint8_t *src = bm->buffer + row * bm->pitch;
        uint32_t *dst = (uint32_t*)((uint8_t*)buf->data + y * buf->stride) + dst_x;

        for (int col = 0; col < (int)bm->width; col++) {
            int x = dst_x + col;
            if (x < 0 || x >= buf->width) continue;

            uint8_t cov = src[col];
            uint32_t old = dst[col];
            dst[col] = blend_coverage_over_xrgb(old, cov, fr, fg, fb);
        }
    }
}

static void draw_ascii_text_freetype(
    struct shm_buffer *buf,
    FT_Face face,
    int baseline_x,
    int baseline_y,
    const char *text,
    uint8_t fr, uint8_t fg, uint8_t fb
) {
    int pen_x = baseline_x;
    int pen_y = baseline_y;

    for (const unsigned char *p = (const unsigned char*)text; *p; p++) {
        unsigned char c = *p;

        if (c == '\n') {
            pen_x = baseline_x;
            // crude line advance: use face metrics
            int line = (int)(face->size->metrics.height >> 6);
            if (line <= 0) line = 16;
            pen_y += line;
            continue;
        }

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
            blit_ft_bitmap_xrgb(buf, &g->bitmap, x, y, fr, fg, fb);
        } else {
            // For a first pass, ignore other pixel modes.
        }

        pen_x += (int)(g->advance.x >> 6);
    }
}

FT_Library ft;
FT_Face face;

int initialize_draw() {
    // Font path: pass as argv[1], otherwise use a common default on many Linux distros.
    const char *font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
    
    if (FT_Init_FreeType(&ft) != 0) {
        fprintf(stderr, "FT_Init_FreeType failed\n");
        return 1;
    }

    if (FT_New_Face(ft, font_path, 0, &face) != 0) {
        fprintf(stderr, "FT_New_Face failed (font path: %s)\n", font_path);
        FT_Done_FreeType(ft);
        return 1;
    }

    // Set pixel size (height in px). Width 0 = auto from height.
    if (FT_Set_Pixel_Sizes(face, 0, 16) != 0) {
        fprintf(stderr, "FT_Set_Pixel_Sizes failed\n");
    }

    return 0;
}

void draw(struct shm_buffer *buf) {
    // Background + text
    fill_solid_xrgb(buf, pack_xrgb(0x00, 0x00, 0x00));

    draw_ascii_text_freetype(
        buf, 
        face, 
        20, 
        50,  
        "ABCDEFGHIJKLMNOPQRSUVWXYZ\nabcdefghijklmnopqrstuvwxyz",
        0xBB, 
        0xBB, 
        0xBB
    );
}