#ifndef __FONT_H__
#define __FONT_H__

#include "stm32f4xx.h"
#include "stdint.h"

/* 字体对象：字模表 + 度量打包成一个对象，渲染函数面向对象而非字号分支。
   三套 ASCII 字模(0x20~0x7E, 95 字符)，行列式/阴码/MSB先，索引 = ch-0x20。 */
typedef struct {
    uint8_t w, h, bytes;        /* 字符宽/高/每字符字节数 */
    const uint8_t *tab;         /* 字模表 */
} font_t;

extern const font_t font16;     /* 8x16  */
extern const font_t font20;     /* 16x20 */
extern const font_t font52;     /* 32x52 */

/* 按字高(16/20/52)取字体对象；非法值退回 20 */
const font_t *font_get(uint8_t size);

/* 显示字符串（纯 ASCII）：非可打印字符按空格；超出屏宽自动换行。fg=笔画色，bg=底色。 */
void font_show(const font_t *f, uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg);

/* 透明显示：只画 fg 笔画、不画底色，背景图直接露出。
   动态刷新前须先用 st7789_restore_bg 擦回背景，否则旧字残影。 */
void font_show_transparent(const font_t *f, uint16_t x, uint16_t y, const char *str, uint16_t fg);

#endif /* __FONT_H__ */
