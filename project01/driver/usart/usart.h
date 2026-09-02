#ifndef __USART_H__
#define __USART_H__

#include "stm32f4xx.h"
#include "stdint.h"
#include "stdbool.h"
#include <stdio.h>

void USART1_Init(void);
void USART1_SendByte(uint8_t byte);
void USART1_SendString(const char *str);
void USART1_SendBuffer(uint8_t data[], uint32_t length);

/* 便捷宏：发送整个数组，长度由 sizeof 自动计算（仅限定义处可见的数组） */
#define USART1_SEND_ARRAY(arr)  USART1_SendBuffer((arr), sizeof(arr) / sizeof((arr)[0]))
bool USART1_ReceiveBuffer(uint8_t data[], uint32_t length);

#endif
