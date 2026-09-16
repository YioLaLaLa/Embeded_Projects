#ifndef __SPI_H__
#define __SPI_H__

#include "stm32f4xx.h"
#include "stdint.h"
#include "stdbool.h"

/* SPI 实例描述（handle 模式）：一个结构体描述一路 SPI 的全部配置，
   函数接收它的指针，同一套函数即可驱动 SPI1/SPI2/SPI3 多路。 */
typedef struct {
    SPI_TypeDef  *SPIx;      /* 外设实例：SPI1 / SPI2 / SPI3 */
    uint32_t      prescaler; /* 分频系数：SPI_BaudRatePrescaler_x */
    GPIO_TypeDef *gpio;      /* SCK/MISO/MOSI 所在端口 */
    uint16_t      sck, miso, mosi;              /* 引脚掩码 GPIO_Pin_x */
    uint8_t       sck_src, miso_src, mosi_src;  /* 引脚源 GPIO_PinSourceN */
    uint8_t       af;        /* 复用功能：GPIO_AF_SPIx */
} spi_t;

/* 按结构体配置初始化一路 SPI：主机 / 全双工 / 8 位 / Mode0(CPOL=0,CPHA=0) / MSB / 软件 NSS */
void spi_init(spi_t *s);

/* 全双工收发一个字节：发送 data 的同时返回接收到的字节（SPI 最基本的收发原语）*/
uint8_t spi_rw(spi_t *s, uint8_t data);

/* 连续发送多个字节（丢弃接收到的数据）*/
void spi_send_buf(spi_t *s, const uint8_t *data, uint32_t len);

/* 连续接收多个字节（发送 dummy 0xFF 以产生时钟）*/
void spi_read_buf(spi_t *s, uint8_t *buf, uint32_t len);

#endif /* __SPI_H__ */
