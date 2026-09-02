#include <stdint.h>
#include "stm32f4xx.h"
#include "stdbool.h"
#include "usart.h"
#include "bl24c512.h"

int main(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	USART1_Init();
	BL24C512_Init();

	while(1)
	{;}
}
