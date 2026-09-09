#ifndef __AHT20_H__
#define __AHT20_H__

#include "stm32f4xx.h"
#include "stdint.h"
#include "stdbool.h"

/*
AHT20 温湿度传感器（I2C，7 位地址 0x38）
依赖：driver/I2C（i2c_write/i2c_read）、driver/delay（Delay_ms）
用前须先调用 I2C1_Init() 和 Delay_Init()。
*/

/* 上电初始化：延时稳定 → 读状态 → 若未校准则发初始化命令。成功返回 true */
bool AHT20_Init(void);

/* 触发一次测量并换算：humidity 得到相对湿度 RH%（0~100），temperature 得到摄氏温度 °C。成功返回 true */
bool AHT20_Read(float *humidity, float *temperature);

/* 读状态字节（调试用）：bit7=忙(1 测量中)，bit3=校准(1 已校准)。成功返回 true */
bool AHT20_ReadStatus(uint8_t *status);

#endif
