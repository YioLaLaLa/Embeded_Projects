#ifndef __BSP_IO_H
#define __BSP_IO_H

#include "main.h"
#include <stdint.h>

/* LED 控制接口 */
void LED1_On(void);
void LED1_Off(void);
void LED1_Toggle(void);

void LED2_On(void);
void LED2_Off(void);
void LED2_Toggle(void);

void LED3_On(void);
void LED3_Off(void);
void LED3_Toggle(void);

void LED_AllOff(void);
void LED_AllOn(void);

/* 按键读取接口，返回 1 表示按下，0 表示松开 */
uint8_t KEY1_IsPressed(void);
uint8_t KEY2_IsPressed(void);
uint8_t KEY3_IsPressed(void);

/* 测试函数：按键直接控制 LED */
void IO_TestTask(void);

#endif
