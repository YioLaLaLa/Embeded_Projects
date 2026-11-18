#include "font.h"
#include "st7789.h"

/* 字模表定义在 font16.c / font20.c / font52.c，本文件组装成字体对象 */
extern const uint8_t ascii_font16[];
extern const uint8_t ascii_font20[];
extern const uint8_t ascii_font52[];

const font_t font16 = { 8,  16, 16,  ascii_font16 };
const font_t font20 = { 16, 20, 40,  ascii_font20 };
const font_t font52 = { 32, 52, 208, ascii_font52 };

const font_t *font_get(uint8_t size)
{
    if(size == 16) return &font16;
    if(size == 52) return &font52;
    return &font20;
}

/* 统一渲染内核：transparent=0 带底色，=1 只画笔画 */
static void show(const font_t *f, uint16_t x, uint16_t y, const char *str,
                 uint16_t fg, uint16_t bg, int transparent)
{
    const uint8_t *p = (const uint8_t *)str;
    while(*p)
    {
        char c = (*p < 0x20 || *p > 0x7E) ? ' ' : (char)*p;
        const uint8_t *glyph = &f->tab[(uint16_t)(c - 0x20) * f->bytes];
        if(transparent) st7789_draw_bitmap_transparent(x, y, f->w, f->h, glyph, fg);
        else            st7789_draw_bitmap(x, y, f->w, f->h, glyph, fg, bg);
        x += f->w;
        if(x + f->w > ST7789_WIDTH) { x = 0; y += f->h; }   /* 换行 */
        p++;
    }
}

void font_show(const font_t *f, uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg)
{
    show(f, x, y, str, fg, bg, 0);
}

void font_show_transparent(const font_t *f, uint16_t x, uint16_t y, const char *str, uint16_t fg)
{
    show(f, x, y, str, fg, 0, 1);
}
