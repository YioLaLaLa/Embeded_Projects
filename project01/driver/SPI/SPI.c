#include "stm32f4xx.h"
#include "stdbool.h"
#include "SPI.h"

void spi_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    SPI_InitTypeDef SPI_InitStruct;
    SPI_StructInit(&SPI_InitStruct);
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;   // 测试阶段用 ~1.3MHz(APB1 42MHz/32) 稳定优先；验证通过后可改回 _2/_4 提速
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init(SPI2, &SPI_InitStruct);
    SPI_Cmd(SPI2, ENABLE);
}

/* 全双工收发一个字节：发送 data 的同时返回接收到的字节 */
uint8_t spi_rw(uint8_t data)
{
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);   // 等发送缓冲区空
    SPI_I2S_SendData(SPI2, data);                                    // 写入要发送的字节
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);  // 等接收到一个字节
    return (uint8_t)SPI_I2S_ReceiveData(SPI2);                       // 读出接收到的字节
}

/* 连续发送多个字节（丢弃接收到的数据）*/
void spi_send_buf(const uint8_t *data, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
        (void)spi_rw(data[i]);
}

/* 连续接收多个字节（发送 dummy 0xFF 以产生时钟）*/
void spi_read_buf(uint8_t *buf, uint32_t len)
{
    for(uint32_t i = 0; i < len; i++)
        buf[i] = spi_rw(0xFF);
}
