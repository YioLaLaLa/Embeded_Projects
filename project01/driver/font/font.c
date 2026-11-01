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

/* 按 size(16/32/48) 取对应中文字模表与宽高/字节数；非法值退回 32 */
static void font_cn_metrics(uint8_t size, const uint8_t **tab,
                            uint16_t *w, uint16_t *h, uint16_t *bytes)
{
    switch(size)
    {
        case 16: *tab = chinese_font16; *w = FONT16_CN_W; *h = FONT16_CN_H; *bytes = FONT16_CN_BYTES; break;
        case 48: *tab = chinese_font48; *w = FONT48_CN_W; *h = FONT48_CN_H; *bytes = FONT48_CN_BYTES; break;
        default: *tab = chinese_font32; *w = FONT32_CN_W; *h = FONT32_CN_H; *bytes = FONT32_CN_BYTES; break;
    }
}

/* ===== 码点 → 字库索引 映射表 =====
   左=汉字 GBK 码(高字节<<8 | 低字节)，右=它在 chinese_fontXX[] 里的序号。
   加新字：先在字库数组末尾追加字模(记下序号)，再在这里加一行 {GBK码, 序号}。 */
typedef struct { uint32_t code; uint16_t idx; } cn_map_t;
static const cn_map_t cn_map[] = {
    { 0xC4EA,  0 },   /* 年 */
    { 0xD4C2,  1 },   /* 月 */
    { 0xC8D5,  2 },   /* 日 */
    { 0xCAB1,  3 },   /* 时 */
    { 0xB7D6,  4 },   /* 分 */
    { 0xC3EB,  5 },   /* 秒 */
    { 0xD0C7,  6 },   /* 星 */
    { 0xC6DA,  7 },   /* 期 */
    { 0xD6DC,  8 },   /* 周 */
    { 0xC4A9,  9 },   /* 末 */
    { 0xBDF1, 10 },   /* 今 */
    { 0xC3F7, 11 },   /* 明 */
    { 0xCCEC, 12 },   /* 天 */
    { 0xC9CF, 13 },   /* 上 */
    { 0xCEE7, 14 },   /* 午 */
    { 0xB5E3, 15 },   /* 点 */
    { 0xD2BB, 16 },   /* 一 */
    { 0xB6FE, 17 },   /* 二 */
    { 0xC8FD, 18 },   /* 三 */
    { 0xCBC4, 19 },   /* 四 */
    { 0xCEE5, 20 },   /* 五 */
    { 0xC1F9, 21 },   /* 六 */
    { 0xC6F8, 22 },   /* 气 */
    { 0xCFF3, 23 },   /* 象 */
    { 0xCEC2, 24 },   /* 温 */
    { 0xB6C8, 25 },   /* 度 */
    { 0xCAAA, 26 },   /* 湿 */
    { 0xB7E7, 27 },   /* 风 */
    { 0xCFF2, 28 },   /* 向 */
    { 0xBCB6, 29 },   /* 级 */
    { 0xD1B9, 30 },   /* 压 */
    { 0xC7E7, 31 },   /* 晴 */
    { 0xB6E0, 32 },   /* 多 */
    { 0xD4C6, 33 },   /* 云 */
    { 0xC9D9, 34 },   /* 少 */
    { 0xD2F5, 35 },   /* 阴 */
    { 0xD5F3, 36 },   /* 阵 */
    { 0xD3EA, 37 },   /* 雨 */
    { 0xC0D7, 38 },   /* 雷 */
    { 0xD0A1, 39 },   /* 小 */
    { 0xD6D0, 40 },   /* 中 */
    { 0xB4F3, 41 },   /* 大 */
    { 0xD1A9, 42 },   /* 雪 */
    { 0xBCD0, 43 },   /* 夹 */
    { 0xCEED, 44 },   /* 雾 */
    { 0xF6B2, 45 },   /* 霾 */
    { 0xC9B3, 46 },   /* 沙 */
    { 0xB3BE, 47 },   /* 尘 */
    { 0xB6B3, 48 },   /* 冻 */
    { 0xB1F9, 49 },   /* 冰 */
    { 0xB1A2, 50 },   /* 雹 */
    { 0xB5BD, 51 },   /* 到 */
    { 0xD7AA, 52 },   /* 转 */
    { 0xBED6, 53 },   /* 局 */
    { 0xB2BF, 54 },   /* 部 */
    { 0xB6AB, 55 },   /* 东 */
    { 0xC4CF, 56 },   /* 南 */
    { 0xCEF7, 57 },   /* 西 */
    { 0xB1B1, 58 },   /* 北 */
    { 0xC6AB, 59 },   /* 偏 */
    { 0xB8DF, 60 },   /* 高 */
    { 0xB5CD, 61 },   /* 低 */
    { 0xC1E3, 62 },   /* 零 */
    { 0xC9E3, 63 },   /* 摄 */
    { 0xCACF, 64 },   /* 氏 */
    { 0xB8FC, 65 },   /* 更 */
    { 0xD0C2, 66 },   /* 新 */
    { 0xBCD3, 67 },   /* 加 */
    { 0xD4D8, 68 },   /* 载 */
    { 0xCAA7, 69 },   /* 失 */
    { 0xB0DC, 70 },   /* 败 */
    { 0xC1AC, 71 },   /* 连 */
    { 0xBDD3, 72 },   /* 接 */
    { 0xCEDE, 73 },   /* 无 */
    { 0xD0C5, 74 },   /* 信 */
    { 0xBAC5, 75 },   /* 号 */
};

/* 码点 → 字库索引；查不到返回 0xFFFF */
static uint16_t font_cn_code2idx(uint32_t code)
{
    for(uint16_t i = 0; i < sizeof(cn_map) / sizeof(cn_map[0]); i++)
        if(cn_map[i].code == code) return cn_map[i].idx;
    return 0xFFFF;
}

/* 从 GBK 串解出一个字符码，返回消耗字节数(1~2)。
   ASCII(单字节)返回其本身；汉字(双字节)返回 (高字节<<8 | 低字节)。 */
static uint8_t font_gbk_decode(const uint8_t *s, uint32_t *code)
{
    if(s[0] < 0x80) { *code = s[0]; return 1; }              /* ASCII */
    *code = ((uint32_t)s[0] << 8) | s[1];                    /* GBK 双字节 */
    return 2;
}

/* 显示字符串（中英文混排，源串须为 GBK）：ASCII 走 ascii 字模，汉字查 cn_map→索引 */
void font_show_string(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t size)
{
    const uint8_t *at; uint16_t aw, ah, abytes;   /* ascii 度量 */
    font_metrics(size, &at, &aw, &ah, &abytes);
    const uint8_t *ct; uint16_t cw, ch, cbytes;   /* 中文度量 */
    font_cn_metrics(size, &ct, &cw, &ch, &cbytes);

    const uint8_t *p = (const uint8_t *)str;
    while(*p)
    {
        uint32_t code; uint8_t n = font_gbk_decode(p, &code);
        if(n == 0) { p++; continue; }             /* 非法字节跳过 */

        uint16_t adv;
        if(code < 0x80)                            /* ASCII */
        {
            char c = (code < 0x20 || code > 0x7E) ? ' ' : (char)code;
            st7789_draw_bitmap(x, y, aw, ah, &at[(uint16_t)(c - 0x20) * abytes], fg, bg);
            adv = aw;
        }
        else                                       /* 汉字 */
        {
            uint16_t idx = font_cn_code2idx(code);
            if(idx != 0xFFFF)
                st7789_draw_bitmap(x, y, cw, ch, &ct[(uint32_t)idx * cbytes], fg, bg);
            adv = cw;
        }
        x += adv;
        if(x + adv > ST7789_WIDTH) { x = 0; y += ch; }   /* 换行 */
        p += n;
    }
}
