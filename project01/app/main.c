#include <stdint.h>
#include "stm32f4xx.h"
#include "stdbool.h"
#include "usart.h"
#include "delay.h"
#include "i2c.h"
#include "aht20.h"

/* ===== AHT20 温湿度监视器：每秒通过 USART1(115200 8-N-1) 打印一次 ===== */

/* 串口发送无符号整数（十进制） */
static void USART1_SendNumber(uint32_t n)
{
	uint8_t buf[10], i = 0;
	if(n == 0){ USART1_SendByte('0'); return; }
	while(n){ buf[i++] = (uint8_t)('0' + (n % 10)); n /= 10; }
	while(i) USART1_SendByte(buf[--i]);
}

/* 串口发送保留一位小数的浮点（拆整数+小数，避免依赖 %f） */
static void USART1_SendFloat1(float f)
{
	int32_t i = (int32_t)f;
	int32_t frac;
	if(f < 0){ USART1_SendByte('-'); f = -f; i = -i; }
	frac = (int32_t)((f - (float)i) * 10.0f + 0.5f);   // 第一位小数，四舍五入
	if(frac >= 10){ i += 1; frac = 0; }                 // 进位（如 25.98 -> 26.0）
	USART1_SendNumber((uint32_t)i);
	USART1_SendByte('.');
	USART1_SendByte((uint8_t)('0' + frac));
}

int main(void)
{
	Delay_Init();
	USART1_Init();
	I2C1_Init();

	USART1_SendString("\r\n===== AHT20 Temp/Humidity Monitor =====\r\n");
	USART1_SendString("USART1 115200 8-N-1 | AHT20 on I2C1 (0x38) | 100kHz\r\n\r\n");

	if(AHT20_Init())
		USART1_SendString("AHT20 Init OK\r\n\r\n");
	else
		USART1_SendString("AHT20 Init FAIL\r\n\r\n");

	while(1)
	{
		float humidity, temperature;
		if(AHT20_Read(&humidity, &temperature))       // 注意：湿度在前、温度在后
		{
			USART1_SendString("Temp: ");
			USART1_SendFloat1(temperature);
			USART1_SendString(" C    Hum: ");
			USART1_SendFloat1(humidity);
			USART1_SendString(" %\r\n");
		}
		else
		{
			USART1_SendString("AHT20 read FAIL\r\n");
		}
		Delay_ms(1000);
	}
}
