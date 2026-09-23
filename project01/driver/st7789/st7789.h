#ifndef __ST7789_H__
#define __ST7789_H__

#include "stm32f4xx.h"
#include "stdint.h"

/* ==================== 引脚配置：按你的实际接线改这里 ==================== */
/* SPI2 由 spi_init() 配置：SCK=PB13, MOSI=PB15（LCD 只写，MISO=PB14 不用）。
   控制引脚按板载 TFT 座 P2 实际接线：CS=PB12, DC=PC5, BLK=PB1；RST 不在 P2 上，杜邦线接 PD8（空闲脚）。
   DC 在 GPIOC、RST 在 GPIOD，st7789.c 的 gpio_init 已分别初始化 GPIOB / GPIOC / GPIOD。 */
#define ST7789_CS_PORT    GPIOB
#define ST7789_CS_PIN     GPIO_Pin_12     /* 片选，低有效（P2-5）*/
#define ST7789_DC_PORT    GPIOC
#define ST7789_DC_PIN     GPIO_Pin_5      /* 数据/命令选择：0=命令，1=数据（P2-7）*/
#define ST7789_RST_PORT   GPIOD
#define ST7789_RST_PIN    GPIO_Pin_8      /* 硬件复位，低有效（杜邦线接 PD8，空闲脚无冲突）*/
#define ST7789_BLK_PORT   GPIOB
#define ST7789_BLK_PIN    GPIO_Pin_1      /* 背光，高=点亮（P2-8）*/

/* ==================== 屏幕参数 ==================== */
#define ST7789_WIDTH      240             /* 常见 240x320 / 240x240 / 135x240 */
#define ST7789_HEIGHT     320

/* MADCTL(36h) 方向值：0x00 = 竖屏正显(RGB)。
   横屏/BGR 面板改这里：MV=0x20, MX=0x40, MY=0x80, BGR=0x08（可按需组合）。 */
#define ST7789_ORIENTATION 0x00

/* ==================== 接口函数 ==================== */
void st7789_init(void);                 /* 完整初始化（含 SPI、复位、寄存器配置）*/
void st7789_write_cmd(uint8_t cmd);     /* 写一条命令 */
void st7789_write_data(uint8_t data);   /* 写一个数据字节 */
void st7789_fill(uint16_t color);       /* 用单一颜色(RGB565)填充整屏 */
void st7789_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);  /* 指定矩形区域填色 */
void st7789_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        const uint8_t *bmp, uint16_t fg, uint16_t bg);   /* 单色位图→双色像素 */

/* RGB565 常用颜色 */
#define ST7789_BLACK   0x0000
#define ST7789_RED     0xF800
#define ST7789_GREEN   0x07E0
#define ST7789_BLUE    0x001F
#define ST7789_WHITE   0xFFFF
#define ST7789_YELLOW  0xFFE0
#define ST7789_CYAN    0x07FF

#endif /* __ST7789_H__ */
