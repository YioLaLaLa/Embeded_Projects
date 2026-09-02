#include "usart.h"
#include "stm32f4xx.h"

/*
USART1 初始化：115200 波特率，8 数据位，无校验，1 停止位
引脚：PA9 = TX（复用 AF7），PA10 = RX（复用 AF7）
*/
void USART1_Init(void)
{
	/* 开时钟：USART1 挂在 APB2，GPIOA 挂在 AHB1 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* 配置 PA9/PA10 为复用推挽输出，带上拉 */
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_StructInit(&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

	/* 配置 USART1 参数并使能 */
	USART_InitTypeDef USART_InitStruct;
	USART_StructInit(&USART_InitStruct);
	USART_InitStruct.USART_BaudRate = 115200;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART1, &USART_InitStruct);
	USART_Cmd(USART1, ENABLE);
}

/* 发送单个字节：等待发送数据寄存器空再写入 */
void USART1_SendByte(uint8_t byte)
{
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	USART_SendData(USART1, byte);
}

/* 发送字符串：逐字节发送，遇到 '\0' 结束 */
void USART1_SendString(const char *str)
{
	while(*str)
	{
		USART1_SendByte((uint8_t)*str);
		str++;
	}
}

/* 发送任意长度数据：逐字节发完 data[]（可含 0x00 等二进制内容） */
void USART1_SendBuffer(uint8_t data[], uint32_t length)
{
	for(uint32_t i = 0; i < length; i++)
	{
		USART1_SendByte(data[i]);
	}
}

/* 接收任意长度数据：逐字节收满 data[]，中途超时返回 false（已收部分保留） */
bool USART1_ReceiveBuffer(uint8_t data[], uint32_t length)
{
	for(uint32_t i = 0; i < length; i++)
	{
		uint32_t timeout = 100000;
		while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
		{
			if(--timeout == 0) return false;              // 某字节超时 → 整体失败退出，不卡死程序
		}
		data[i] = (uint8_t)USART_ReceiveData(USART1);     // 读出数据（兼清 RXNE）
	}
	return true;
}

/* printf 重定向：Keil 中勾选 Use MicroLib 后即可用 printf 输出到串口 */
int fputc(int ch, FILE *f)
{
	USART1_SendByte((uint8_t)ch);
	return ch;
}
