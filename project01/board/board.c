#include "board.h"
#include "delay.h"
#include "usart.h"
#include "st7789.h"
#include "esp32.h"
#include "i2c.h"
#include "aht20.h"
#include "rtc.h"

/* 最低层：延时基准 + 调试串口，之后全工程的日志才可用 */
void board_lowlevel_init(void)
{
	Delay_Init();
	USART1_Init();
	USART1_SendString("\r\n===== Weather Clock =====\r\n");
}

/* 板上外设一次性拉起；顺序即依赖顺序：
   LCD 自带 SPI2 初始化 → ESP32 用 USART2 → AHT20 依赖 I2C1 → RTC 独立。
   传感器经 sensor_t 接口访问，换型号只改这里的实例引用。 */
void board_init(void)
{
	uint8_t rtc_src = 0;

	st7789_init();                    /* LCD 面板 + SPI2 + 背光 */

	esp32_init(&esp32_modem);         /* ESP32-C3 AT 链路所用 USART2（实例化 modem）*/

	i2c_init(&i2c1_bus);              /* I2C1 总线(PB6/PB7)，内部含总线恢复 */
	if(aht20_sensor.init())
		USART1_SendString("[AHT20] init OK\r\n");
	else
		USART1_SendString("[AHT20] init FAIL\r\n");

	if(rtc_init(&rtc_src))
		USART1_SendString(rtc_src ? "[RTC] init OK (LSI)\r\n" : "[RTC] init OK (LSE)\r\n");
	else
		USART1_SendString("[RTC] init FAIL\r\n");
}
