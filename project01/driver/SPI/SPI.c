#include "stm32f4xx.h"
#include "stdbool.h"
#include <stddef.h>   /* NULL */
#include "SPI.h"

/* 使能结构体所指 SPI 外设的时钟：SPI1 在 APB2，SPI2/SPI3 在 APB1 */
static void spi_rcc_enable(SPI_TypeDef *SPIx)
{
    if(SPIx == SPI1)      RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    else if(SPIx == SPI2) RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    else                  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
}

/* 使能结构体所指 GPIO 端口的时钟 */
static void gpio_rcc_enable(GPIO_TypeDef *gpio)
{
    if(gpio == GPIOA)      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    else if(gpio == GPIOB) RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    else if(gpio == GPIOC) RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    else if(gpio == GPIOD) RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    else                   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
}

void spi_init(spi_t *s)
{
    spi_rcc_enable(s->SPIx);
    gpio_rcc_enable(s->gpio);

    GPIO_PinAFConfig(s->gpio, s->sck_src,  s->af);
    GPIO_PinAFConfig(s->gpio, s->miso_src, s->af);
    GPIO_PinAFConfig(s->gpio, s->mosi_src, s->af);

    GPIO_InitTypeDef g;
    GPIO_StructInit(&g);
    g.GPIO_Pin   = s->sck | s->miso | s->mosi;
    g.GPIO_Mode  = GPIO_Mode_AF;
    g.GPIO_Speed = GPIO_High_Speed;
    g.GPIO_OType = GPIO_OType_PP;
    g.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(s->gpio, &g);

    SPI_InitTypeDef spi;
    SPI_StructInit(&spi);
    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_1Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = s->prescaler;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init(s->SPIx, &spi);
    SPI_Cmd(s->SPIx, ENABLE);
}

/* 全双工收发一个字节：发送 data 的同时返回接收到的字节 */
uint8_t spi_rw(spi_t *s, uint8_t data)
{
    while(SPI_I2S_GetFlagStatus(s->SPIx, SPI_I2S_FLAG_TXE) == RESET);   // 等发送缓冲区空
    SPI_I2S_SendData(s->SPIx, data);                                    // 写入要发送的字节
    while(SPI_I2S_GetFlagStatus(s->SPIx, SPI_I2S_FLAG_RXNE) == RESET);  // 等接收到一个字节
    return (uint8_t)SPI_I2S_ReceiveData(s->SPIx);                       // 读出接收到的字节
}

/* 连续发送多个字节（丢弃接收到的数据）*/
void spi_send_buf(spi_t *s, const uint8_t *data, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
        (void)spi_rw(s, data[i]);
}

/* 连续接收多个字节（发送 dummy 0xFF 以产生时钟）*/
void spi_read_buf(spi_t *s, uint8_t *buf, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
        buf[i] = spi_rw(s, 0xFF);
}

/* ==================== DMA 发送（SPI2_TX = DMA1 Stream4 Channel0）====================
   固定 8 位帧 + 字节 DMA：不碰 DFF，避开 F4 在 8 位模式下写半字 DR 打包为
   “低字节先发两帧”的陷阱（RM0090 §28.5.9）。字节序由调用方预先排好。 */

/* 清 RX 残留：先排空 RXNE，再按 RM0090 序列(读 DR 后读 SR)解除 OVR */
void spi_flush_rx(spi_t *s)
{
    while(SPI_I2S_GetFlagStatus(s->SPIx, SPI_I2S_FLAG_RXNE) != RESET)
        (void)s->SPIx->DR;
    if(SPI_I2S_GetFlagStatus(s->SPIx, SPI_I2S_FLAG_OVR) != RESET) {
        (void)s->SPIx->DR;
        (void)s->SPIx->SR;
    }
}

bool spi_dma_send8(spi_t *s, const uint8_t *data, uint32_t len)
{
    DMA_Stream_TypeDef *stream;
    uint32_t flag_tc;

    if(s->SPIx == SPI2) {                     /* 目前只建立 SPI2 的流映射，其余回退轮询 */
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
        stream  = DMA1_Stream4;               /* SPI2_TX = DMA1 Stream4 Ch0 */
        flag_tc = DMA_FLAG_TCIF4;
    } else {
        return false;
    }
    if(data == NULL || len == 0) return true;

    SPI_DMACmd(s->SPIx, SPI_DMAReq_Tx, ENABLE);   /* 请求常开：流不使能时不产生请求 */

    while(len)
    {
        uint32_t n = (len > 65535) ? 65535 : len;   /* NDTR 仅 16 位，大块自动分块 */

        DMA_Cmd(stream, DISABLE);
        while(DMA_GetCmdStatus(stream) != DISABLE);

        DMA_InitTypeDef d;
        DMA_StructInit(&d);                               /* 默认：直接模式/单拍 */
        d.DMA_Channel              = DMA_Channel_0;
        d.DMA_PeripheralBaseAddr   = (uint32_t)&s->SPIx->DR;
        d.DMA_Memory0BaseAddr      = (uint32_t)data;
        d.DMA_DIR                  = DMA_DIR_MemoryToPeripheral;
        d.DMA_BufferSize           = n;
        d.DMA_PeripheralInc        = DMA_PeripheralInc_Disable;
        d.DMA_MemoryInc            = DMA_MemoryInc_Enable;
        d.DMA_PeripheralDataSize   = DMA_PeripheralDataSize_Byte;   /* 8 位帧=字节写 DR */
        d.DMA_MemoryDataSize       = DMA_MemoryDataSize_Byte;
        d.DMA_Mode                 = DMA_Mode_Normal;
        d.DMA_Priority             = DMA_Priority_High;
        DMA_Init(stream, &d);

        DMA_ClearFlag(stream, flag_tc);
        DMA_Cmd(stream, ENABLE);
        while(DMA_GetFlagStatus(stream, flag_tc) == RESET);       /* 本块搬完 */
        DMA_ClearFlag(stream, flag_tc);
        DMA_Cmd(stream, DISABLE);

        while(SPI_I2S_GetFlagStatus(s->SPIx, SPI_I2S_FLAG_TXE) == RESET);   /* DR 腾空 */
        while(SPI_I2S_GetFlagStatus(s->SPIx, SPI_I2S_FLAG_BSY) != RESET);  /* 等移出移位器 */

        data += n;
        len  -= n;
    }
    return true;
}
