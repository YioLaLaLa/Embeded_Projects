#ifndef __I2C_H__
#define __I2C_H__

#include "stm32f4xx.h"
#include "stdint.h"
#include "stdbool.h"

/* I2C 实例描述（handle 模式）：一个结构体描述一路 I2C 的全部配置，
   函数接收它的指针，同一套函数即可驱动 I2C1/I2C2/I2C3 多路。 */
typedef struct {
	I2C_TypeDef  *I2Cx;      /* 外设实例：I2C1 / I2C2 / I2C3 */
	uint32_t      speed;     /* SCL 频率：100000 标准模式 / 400000 快速模式 */
	GPIO_TypeDef *gpio;      /* SCL/SDA 所在端口 */
	uint16_t      scl, sda;             /* 引脚掩码 GPIO_Pin_x */
	uint8_t       scl_src, sda_src;     /* 引脚源 GPIO_PinSourceN */
	uint8_t       af;        /* 复用功能：GPIO_AF_I2Cx */
} i2c_t;

/* 现成的 I2C1 总线实例：PB6=SCL，PB7=SDA，AF4，100kHz（定义在 i2c.c）。
   AHT20、BL24C512 都挂在它上面；要用别的引脚就自己再建一个 i2c_t。 */
extern i2c_t i2c1_bus;

/* 按结构体配置初始化一路 I2C：主机 / 7 位地址 / ACK 使能 / 开漏上拉。内部会先做一次总线恢复 */
void i2c_init(i2c_t *s);

/* I2C 总线恢复：手动打 SCL 脉冲解除从机拉死 SDA 造成的 BUSY 锁死（i2c_init 内部已调用） */
void i2c_bus_recovery(i2c_t *s);

/* 通用等待：等待该路 I2C 的 flag 达到 target 状态；超时或从机 NACK 则发 STOP 并返回 false */
bool i2c_wait_flag(i2c_t *s, uint32_t flag, FlagStatus target);

/*
通用 I2C 写：START → 设备地址(写) → 发送 len 字节 → STOP，全程带超时/NACK 保护。
dev_addr 为 8 位地址（7 位地址左移 1 位，如 AHT20=0x70、BL24C512=0xA0）。
*/
bool i2c_write(i2c_t *s, uint8_t dev_addr, const uint8_t *data, uint32_t len);

/*
通用 I2C 读：START → 设备地址(读) → 接收 len 字节 → STOP。
单字节/多字节的 ACK、STOP 时序按手册处理（最后一字节前关 ACK 再 STOP）。
*/
bool i2c_read(i2c_t *s, uint8_t dev_addr, uint8_t *buf, uint32_t len);

#endif
