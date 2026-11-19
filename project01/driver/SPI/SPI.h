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

/* ---- DMA 发送扩展（目前仅支持 SPI2_TX = DMA1 Stream4 Ch0）----
   注意：不切换帧宽，SPI 始终 8 位——F4 在 8 位模式下向 DR 写半字会打包成
   两帧且低字节先发(RM0090 §28.5.9)，若 DFF 切换未按预期生效会导致每像素
   两字节颠倒(颜色全错)。调用方应先把数据排成目标字节序再交给本函数。 */

/* 阻塞式 DMA 发送字节流：len 超 65535 自动分块；返回时已全部移位完成，
   可立即拉高 CS。不支持 DMA 的 SPI 返回 false 且不发送，调用方回退轮询。 */
bool spi_dma_send8(spi_t *s, const uint8_t *data, uint32_t len);

/* 清空 RXNE 并解除 OVR：全双工下只发不收会 RX 溢出，DMA 突发结束后调用一次 */
void spi_flush_rx(spi_t *s);

#endif /* __SPI_H__ */
