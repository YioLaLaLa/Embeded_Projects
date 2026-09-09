#ifndef __DELAY_H__
#define __DELAY_H__

#include "stm32f4xx.h"
#include "stdint.h"

/* 初始化 SysTick 延时（用前必须调用一次，通常在 main 最开头） */
void Delay_Init(void);

/* 微秒级忙等待延时（阻塞式，不用中断/操作系统） */
void Delay_us(uint32_t nus);

/* 毫秒级忙等待延时（阻塞式） */
void Delay_ms(uint32_t nms);

#endif
