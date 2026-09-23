#include <stdint.h>
#include "stm32f4xx.h"
#include "delay.h"
#include "usart.h"
#include "st7789.h"
#include "font.h"

/* ===== ST7789 LCD 字符显示测试：16x32 字库 ===== */

int main(void)
{
	Delay_Init();          /* st7789_init 依赖 Delay_ms，必须先初始化 */
	USART1_Init();         /* 调试串口，打印状态 */

	USART1_SendString("\r\n===== ST7789 LCD Test =====\r\n");
	USART1_SendString("SPI2: SCK=PB13 MOSI=PB15 | CS=PB12 DC=PC5 RST=PD8 BLK=PB1\r\n");

	st7789_init();
	USART1_SendString("ST7789 init done, showing text...\r\n");

	st7789_fill(ST7789_BLACK);                                          /* 清屏 */
	font_show_string(10,  10,  "Size 16", ST7789_WHITE, ST7789_BLACK, 16);
	font_show_string(10,  40,  "Size 32", ST7789_RED,   ST7789_BLACK, 32);
	font_show_string(10,  90,  "Size 48", ST7789_GREEN, ST7789_BLACK, 48);

	while(1)
	{
		/* 静态画面，空转 */
	}
}
