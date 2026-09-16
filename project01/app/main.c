#include <stdint.h>
#include "stm32f4xx.h"
#include "delay.h"
#include "usart.h"
#include "st7789.h"

/* ===== ST7789 LCD 点亮测试：初始化后循环刷 红→绿→蓝→白 ===== */

int main(void)
{
	Delay_Init();          /* st7789_init 依赖 Delay_ms，必须先初始化 */
	USART1_Init();         /* 调试串口，打印状态 */

	USART1_SendString("\r\n===== ST7789 LCD Test =====\r\n");
	USART1_SendString("SPI2: SCK=PB13 MOSI=PB15 | CS=PB12 DC=PB1 RST=PB10 BLK=PB11\r\n");

	st7789_init();
	USART1_SendString("ST7789 init done, filling colors...\r\n");

	while(1)
	{
		st7789_fill(ST7789_RED);    Delay_ms(800);
		st7789_fill(ST7789_GREEN);  Delay_ms(800);
		st7789_fill(ST7789_BLUE);   Delay_ms(800);
		st7789_fill(ST7789_WHITE);  Delay_ms(800);
	}
}
