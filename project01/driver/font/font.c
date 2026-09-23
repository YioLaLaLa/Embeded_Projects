#include "font.h"
#include "st7789.h"

/* 按 size(16/32/48) 取对应字模表与宽高/字节数；非法值退回 16 */
static void font_metrics(uint8_t size, const uint8_t **tab,
                         uint16_t *w, uint16_t *h, uint16_t *bytes)
{
    switch(size)
    {
        case 32: *tab = ascii_font32; *w = FONT32_W; *h = FONT32_H; *bytes = FONT32_BYTES; break;
        case 48: *tab = ascii_font48; *w = FONT48_W; *h = FONT48_H; *bytes = FONT48_BYTES; break;
        default: *tab = ascii_font16; *w = FONT16_W; *h = FONT16_H; *bytes = FONT16_BYTES; break;
    }
}

/* 在 (x,y) 显示一个 ASCII 字符，size=16/32/48 选字高 */
void font_show_char(uint16_t x, uint16_t y, char ch, uint16_t fg, uint16_t bg, uint8_t size)
{
    if(ch < 0x20 || ch > 0x7E) ch = ' ';              /* 不可打印字符按空格处理 */
    const uint8_t *tab; uint16_t w, h, bytes;
    font_metrics(size, &tab, &w, &h, &bytes);
    st7789_draw_bitmap(x, y, w, h, &tab[(uint16_t)(ch - 0x20) * bytes], fg, bg);
}

/* 从 (x,y) 起逐个字符显示字符串，到右边界自动换行 */
void font_show_string(uint16_t x, uint16_t y, const char *s, uint16_t fg, uint16_t bg, uint8_t size)
{
    const uint8_t *tab; uint16_t w, h, bytes;
    font_metrics(size, &tab, &w, &h, &bytes);
    while(*s)
    {
        font_show_char(x, y, *s, fg, bg, size);
        x += w;
        if(x + w > ST7789_WIDTH) { x = 0; y += h; }   /* 换行 */
        s++;
    }
}
