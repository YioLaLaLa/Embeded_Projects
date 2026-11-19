#ifndef __ESP32_H__
#define __ESP32_H__

#include "stm32f4xx.h"
#include "stdint.h"
#include "stdbool.h"
#include "datetime.h"

/* ESP32-C3(AT 固件) 经 USART2 连接：
   STM32 PA2(TX) → ESP32 GPIO6(RX)；STM32 PA3(RX) ← ESP32 GPIO7(TX)；共地。
   波特率按你 ESP32 AT 固件实际值改这里(默认 115200)。 */
#define ESP32_BAUDRATE   115200

/* RX 链路：USART2_RX DMA(DMA1 Stream5 Ch4) 循环收进 rx_dma，
   IDLE 中断(USART2_IRQHandler，定义在 esp32.c)把新到字节搬进 rx_ring，
   读取侧只从内存环取字节——不丢字节、不占 RXNE 轮询。 */
#define ESP32_RX_DMA_SIZE   512    /* DMA 循环缓冲(字节) */
#define ESP32_RX_RING_SIZE  2048   /* 软件环形缓冲：需容下整包 HTTP 回复(~1200B) */

/* modem 对象：USART 实例 + AT 交互工作缓冲 + RX DMA 状态（原文件 static，现入实例成员）。
   方法一律带句柄首参；esp32_modem 是 USART2 上唯一实例。 */
typedef struct {
    USART_TypeDef *usart;       /* 所挂串口，init 时赋值 */
    char cmd[128];              /* AT 指令拼装缓冲 */
    char req[256];              /* HTTP 请求报文缓冲 */
    char tmp[192];              /* AT 回复/中转缓冲 */
    /* RX DMA：rx_dma 由 DMA 硬件写；IDLE ISR 搬运至 rx_ring；读取侧只动 rx_tail */
    uint8_t rx_dma[ESP32_RX_DMA_SIZE];
    char rx_ring[ESP32_RX_RING_SIZE];
    volatile uint16_t rx_head;      /* 环写指针(ISR 更新) */
    volatile uint16_t rx_tail;      /* 环读指针(主循环更新) */
    volatile uint16_t rx_dma_pos;   /* ISR 已搬运到的 DMA 缓冲位置 */
} esp32_t;

extern esp32_t esp32_modem;     /* USART2 上的 ESP32 实例（定义在 esp32.c）*/

/* 初始化实例所挂串口（USART2，PA2/PA3）及 RX DMA + IDLE 中断。
   须在 Delay_Init 之后调用（读取超时用 Delay_us 计时）。 */
void esp32_init(esp32_t *e);

/* 向 ESP32 原样发送一个字符串（不自动加 \r\n）*/
void esp32_send(esp32_t *e, const char *s);

/* 发送一条 AT 指令(自动补 \r\n)，把回复收进 resp 并补 '\0'。
   回复中出现 "OK" 返回 true；出现 "ERROR" 或超时返回 false。resp_size 建议 ≥128。 */
bool esp32_at(esp32_t *e, const char *cmd, char *resp, uint16_t resp_size);

/* 连 WiFi：设 STA 模式 + AT+CWJAP。成功返回 true，过程回复存 resp(建议≥192)。 */
bool esp32_connect_wifi(esp32_t *e, const char *ssid, const char *pwd, char *resp, uint16_t resp_size);

/* 经 TCP 向 host:80 发 HTTP GET(path)，原始回复(含HTTP头)收进 resp(建议≥600)。
   仅 HTTP 明文(MINI 固件不支持 HTTPS)。回复含 "200" 返回 true。 */
bool esp32_http_get(esp32_t *e, const char *host, const char *path, char *resp, uint16_t resp_size);

/* 查询本机(STA) IP（发 AT+CIFSR 解析），存入 ip。成功返回 true。 */
bool esp32_get_ip(esp32_t *e, char *ip, uint16_t size);

/* 配置 SNTP 对时：tz=时区(北京=8)，server 如 "ntp.aliyun.com"。成功 true。 */
bool esp32_sntp_cfg(esp32_t *e, int tz, const char *server);

/* 查询 SNTP 当前时间并解析进 dt(未同步/1970 返回 false)。成功 true。 */
bool esp32_sntp_read(esp32_t *e, DateTime *dt);

#endif /* __ESP32_H__ */
