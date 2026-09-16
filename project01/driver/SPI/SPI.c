#include "stm32f4xx.h"
#include "stdbool.h"
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
