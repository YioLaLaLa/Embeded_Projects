#ifndef __SPI_H__
#define __SPI_H__

#include "stm32f4xx.h"
#include "stdint.h"
#include "stdbool.h"

/* 初始化 SPI2：主机 / 全双工 / 8 位 / Mode0(CPOL=0, CPHA=0) / MSB / 软件 NSS
   引脚映射：PB13 = SCK，PB14 = MISO，PB15 = MOSI */
void spi_init(void);

/* 全双工收发一个字节：发送 data 的同时返回接收到的字节（SPI 最基本的收发原语）*/
uint8_t spi_rw(uint8_t data);

/* 连续发送多个字节（丢弃接收到的数据）*/
void spi_send_buf(const uint8_t *data, uint32_t len);

/* 连续接收多个字节（发送 dummy 0xFF 以产生时钟）*/
void spi_read_buf(uint8_t *buf, uint32_t len);

#endif /* __SPI_H__ */
