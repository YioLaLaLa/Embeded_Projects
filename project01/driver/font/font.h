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

extern const uint8_t ascii_font16[];              /* 8x16  字模表 */
extern const uint8_t ascii_font32[];              /* 16x32 字模表 */
extern const uint8_t ascii_font48[];              /* 24x48 字模表 */

/* size 取 16 / 32 / 48 选字模高度；fg=笔画色，bg=底色 */
void font_show_char(uint16_t x, uint16_t y, char ch, uint16_t fg, uint16_t bg, uint8_t size);
void font_show_string(uint16_t x, uint16_t y, const char *s, uint16_t fg, uint16_t bg, uint8_t size);

#endif /* __FONT_H__ */
