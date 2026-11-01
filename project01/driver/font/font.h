#ifndef __FONT_H__
#define __FONT_H__

#include "stm32f4xx.h"
#include "stdint.h"

/* 三套 ASCII 字模(0x20~0x7E, 95 字符)，行列式/阴码/MSB先，索引 = ch-0x20。
   命名按字高：font16=8x16, font32=16x32, font48=24x48。 */
#define FONT16_W     8
#define FONT16_H     16
#define FONT16_BYTES (FONT16_W / 8 * FONT16_H)     /* 16  字节/字符 */

#define FONT32_W     16
#define FONT32_H     32
#define FONT32_BYTES (FONT32_W / 8 * FONT32_H)     /* 64  字节/字符 */

#define FONT48_W     24
#define FONT48_H     48
#define FONT48_BYTES (FONT48_W / 8 * FONT48_H)     /* 144 字节/字符 */

/* 中文字模：16/32/48 三套，行列式/阴码/MSB先，索引由外部编码表决定。
   中文按全宽：字宽=字高，故 16=16x16, 32=32x32, 48=48x48。 */
#define FONT16_CN_W     16
#define FONT16_CN_H     16
#define FONT16_CN_BYTES (FONT16_CN_W / 8 * FONT16_CN_H)   /* 32  字节/字 */

#define FONT32_CN_W     32
#define FONT32_CN_H     32
#define FONT32_CN_BYTES (FONT32_CN_W / 8 * FONT32_CN_H)   /* 128 字节/字 */

#define FONT48_CN_W     48
#define FONT48_CN_H     48
#define FONT48_CN_BYTES (FONT48_CN_W / 8 * FONT48_CN_H)   /* 288 字节/字 */

extern const uint8_t ascii_font16[];              /* 8x16  字模表 */
extern const uint8_t ascii_font32[];              /* 16x32 字模表 */
extern const uint8_t ascii_font48[];              /* 24x48 字模表 */
extern const uint8_t chinese_font16[];            /* 16x16 中文字模表（待添加）*/
extern const uint8_t chinese_font32[];            /* 32x32 中文字模表(你/好...) */
extern const uint8_t chinese_font48[];            /* 48x48 中文字模表（待添加）*/

/* 显示字符串（中英文混排，源串须为 GBK），size=16/32/48 选字模高度；fg=笔画色，bg=底色。
   ASCII 用 ascii 字库；汉字经 cn_map(GBK码→索引) 查中文字库。 */
void font_show_string(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t size);

#endif /* __FONT_H__ */
