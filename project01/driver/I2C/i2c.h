#ifndef __I2C_H__
#define __I2C_H__

#include "stm32f4xx.h"
#include "stdint.h"
#include "stdbool.h"

/* I2C1 总线初始化：PB6 = SCL，PB7 = SDA（复用 AF4、开漏、上拉）。内部会先做一次总线恢复 */
void I2C1_Init(void);

/* I2C 总线恢复：手动打 SCL 脉冲解除从机拉死 SDA 造成的 BUSY 锁死（I2C1_Init 内部已调用） */
void I2C1_BusRecovery(void);

/* 通用等待：等待 I2Cx 的 flag 达到 target 状态；超时或从机 NACK 则恢复总线并返回 false */
bool i2c_wait_flag(I2C_TypeDef *I2Cx, uint32_t flag, FlagStatus target);

/*
通用 I2C 写：START → 设备地址(写) → 发送 len 字节 → STOP，全程带超时/NACK 保护。
dev_addr 为 8 位地址（7 位地址左移 1 位，如 AHT20=0x70、BL24C512=0xA0）。
*/
bool i2c_write(I2C_TypeDef *I2Cx, uint8_t dev_addr, const uint8_t *data, uint32_t len);

/*
通用 I2C 读：START → 设备地址(读) → 接收 len 字节 → STOP。
单字节/多字节的 ACK、STOP 时序按手册处理（最后一字节前关 ACK 再 STOP）。
*/
bool i2c_read(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t *buf, uint32_t len);

#endif
