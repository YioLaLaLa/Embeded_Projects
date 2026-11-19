#include "esp32.h"
#include "stm32f4xx.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>

/* USART2 上唯一的 modem 实例：AT 工作缓冲全部收进实例成员（原文件级 static）。
   全局变量零初始化，usart 在 esp32_init 里赋值。 */
esp32_t esp32_modem;

/* ===== USART2 底层：PA2=TX(AF7), PA3=RX(AF7)，USART2 挂在 APB1 =====
   RX 由 DMA1 Stream5 循环收进 e->rx_dma，IDLE 中断搬进 e->rx_ring；
   读取侧只从内存环取字节，不碰 RXNE。 */

/* 从接收环取一个字节；timeout 单位微秒(50µs 粒度轮询)。环空则等到超时。 */
static bool esp32_read_byte(esp32_t *e, uint8_t *b, uint32_t timeout_us)
{
    while(timeout_us)
    {
        if(e->rx_tail != e->rx_head)
        {
            *b = (uint8_t)e->rx_ring[e->rx_tail];
            e->rx_tail = (uint16_t)((e->rx_tail + 1) % ESP32_RX_RING_SIZE);
            return true;
        }
        Delay_us(50);
        timeout_us = (timeout_us > 50) ? (timeout_us - 50) : 0;
    }
    return false;
}

/* 丢弃已收数据：清软件环 + 同步 DMA 位置(丢弃 DMA 缓冲里尚未搬运的残留)。
   发新命令前调用，防上一条回复的尾字节被误判进新回复。 */
static void esp32_rx_flush(esp32_t *e)
{
    NVIC_DisableIRQ(USART2_IRQn);   /* 短暂停搬运，防两个指针与 ISR 并发更新 */
    e->rx_tail = e->rx_head;
    e->rx_dma_pos = (uint16_t)((ESP32_RX_DMA_SIZE - DMA_GetCurrDataCounter(DMA1_Stream5)) % ESP32_RX_DMA_SIZE);
    NVIC_EnableIRQ(USART2_IRQn);
}

/* 持续读入 buf，直到出现 endmark / buf 满 / 每字节超时；返回已读长度并补 '\0'。
   endmark=NULL 时只按超时收。per_byte_to 是相邻两字节间允许的最大等待(微秒)。 */
static uint16_t esp32_read_until(esp32_t *e, const char *endmark, char *buf, uint16_t size, uint32_t per_byte_to)
{
    uint16_t n = 0;
    if(buf == NULL || size == 0) return 0;
    buf[0] = '\0';
    while(n < size - 1)
    {
        uint8_t b;
        if(!esp32_read_byte(e, &b, per_byte_to)) break;
        buf[n++] = (char)b;
        buf[n] = '\0';
        if(endmark != NULL && strstr(buf, endmark) != NULL) break;
    }
    return n;
}

void esp32_init(esp32_t *e)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);   /* USART2 在 APB1 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    /* PA2/PA3 复用推挽、带上拉 */
    GPIO_InitTypeDef g;
    GPIO_StructInit(&g);
    g.GPIO_Pin   = GPIO_Pin_2 | GPIO_Pin_3;
    g.GPIO_Mode  = GPIO_Mode_AF;
    g.GPIO_Speed = GPIO_High_Speed;
    g.GPIO_OType = GPIO_OType_PP;
    g.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &g);

    /* 复用映射到 USART2：必须用 GPIO_PinSourceX（引脚号），不是 GPIO_Pin_X（位掩码）*/
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    USART_InitTypeDef u;
    USART_StructInit(&u);
    u.USART_BaudRate            = ESP32_BAUDRATE;
    u.USART_WordLength          = USART_WordLength_8b;
    u.USART_StopBits            = USART_StopBits_1;
    u.USART_Parity              = USART_Parity_No;
    u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    u.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2, &u);
    USART_Cmd(USART2, ENABLE);

    e->usart = USART2;                                 /* 实例绑定所挂串口 */

    /* ---- RX DMA：DMA1 Stream5 Ch4 循环收，IDLE 中断搬进软件环 ---- */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
    DMA_Cmd(DMA1_Stream5, DISABLE);
    DMA_InitTypeDef d;
    DMA_StructInit(&d);
    d.DMA_Channel            = DMA_Channel_4;          /* USART2_RX = DMA1 Stream5 Ch4 */
    d.DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR;
    d.DMA_Memory0BaseAddr    = (uint32_t)e->rx_dma;
    d.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    d.DMA_BufferSize         = ESP32_RX_DMA_SIZE;
    d.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    d.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    d.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    d.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    d.DMA_Mode               = DMA_Mode_Circular;      /* 循环：永不需重启，满 512 自动回卷 */
    d.DMA_Priority           = DMA_Priority_High;
    DMA_Init(DMA1_Stream5, &d);
    DMA_Cmd(DMA1_Stream5, ENABLE);
    USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);

    /* 总线空闲中断：一串字节收完后一次性搬运，读取侧零丢字节 */
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
    NVIC_InitTypeDef nv;
    nv.NVIC_IRQChannel                   = USART2_IRQn;
    nv.NVIC_IRQChannelPreemptionPriority = 1;
    nv.NVIC_IRQChannelSubPriority        = 0;
    nv.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nv);
}

void esp32_send(esp32_t *e, const char *s)
{
    while(*s)
    {
        while(USART_GetFlagStatus(e->usart, USART_FLAG_TXE) == RESET);
        USART_SendData(e->usart, (uint8_t)*s++);
    }
}

/* 内部：发 AT 指令(补 \r\n)，读到出现 OK 或 ERROR 为止。含 OK 且无 ERROR 返回 true。
   per_byte_to 越大，越能容忍慢指令(如 CWJAP 联网要几秒)。单位：微秒。 */
static bool esp32_at_to(esp32_t *e, const char *cmd, char *resp, uint16_t resp_size, uint32_t per_byte_to)
{
    /* 丢弃残留接收数据(环 + DMA 缓冲) */
    esp32_rx_flush(e);

    esp32_send(e, cmd);
    esp32_send(e, "\r\n");

    if(resp == NULL || resp_size < 2)
    {
        uint8_t b; esp32_read_byte(e, &b, per_byte_to);   /* 不关心内容，简单消费一下 */
        return false;
    }
    /* 逐字节收，直到出现 OK / ERROR */
    uint16_t n = 0; uint8_t b;
    resp[0] = '\0';
    while(n < resp_size - 1)
    {
        if(!esp32_read_byte(e, &b, per_byte_to)) break;
        resp[n++] = (char)b;
        resp[n] = '\0';
        if(strstr(resp, "OK") != NULL || strstr(resp, "ERROR") != NULL) break;
    }
    return (strstr(resp, "OK") != NULL) && (strstr(resp, "ERROR") == NULL);
}

bool esp32_at(esp32_t *e, const char *cmd, char *resp, uint16_t resp_size)
{
    return esp32_at_to(e, cmd, resp, resp_size, 1000000);   /* 普通快指令 1s 超时 */
}

/* 连 WiFi：设 STA 模式 + AT+CWJAP。成功返回 true，过程回复存 resp。
   注意：CWJAP 联网可能要几秒，故用较大超时。 */
bool esp32_connect_wifi(esp32_t *e, const char *ssid, const char *pwd, char *resp, uint16_t resp_size)
{
    esp32_at_to(e, "AT+CWMODE=1", e->tmp, sizeof(e->tmp), 1000000);    /* STA 模式 */
    snprintf(e->cmd, sizeof(e->cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
    bool ok = esp32_at_to(e, e->cmd, e->tmp, sizeof(e->tmp), 15000000);   /* 联网 15s，超时要大 */
    if(resp != NULL && resp_size > 0) snprintf(resp, resp_size, "%s", e->tmp);
    return ok;
}

/* 通过 TCP 向 host:80 发 HTTP GET(path)，把服务器原始回复(含HTTP头)收进 resp。
   回复里出现 "200" 视为成功。resp_size 建议 ≥600。
   仅适用 HTTP 明文(MINI 固件不支持 SSL/HTTPS)。 */
bool esp32_http_get(esp32_t *e, const char *host, const char *path, char *resp, uint16_t resp_size)
{
    /* 组装 HTTP 请求报文（实例成员 req）*/
    int reqlen = snprintf(e->req, sizeof(e->req),
                          "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
                          path, host);
    if(reqlen <= 0 || reqlen >= (int)sizeof(e->req)) return false;

    /* 关掉可能残留的旧连接(失败无妨) */
    esp32_at(e, "AT+CIPCLOSE", e->tmp, sizeof(e->tmp));

    /* 建 TCP 连接到 80 端口 */
    snprintf(e->cmd, sizeof(e->cmd), "AT+CIPSTART=\"TCP\",\"%s\",80", host);
    if(!esp32_at_to(e, e->cmd, e->tmp, sizeof(e->tmp), 8000000))       /* TCP 建连 8s */
    {
        if(resp) snprintf(resp, resp_size, "CIPSTART fail: %s", e->tmp);
        return false;
    }

    /* 声明发送字节数，等待 ">" 提示符 */
    snprintf(e->cmd, sizeof(e->cmd), "AT+CIPSEND=%d", reqlen);
    esp32_send(e, e->cmd);
    esp32_send(e, "\r\n");
    esp32_read_until(e, ">", e->tmp, sizeof(e->tmp), 2000000);         /* 等 ">" 提示符 2s */

    /* 发原始 HTTP 报文，再收完整回复(直到 CLOSED，10s) */
    esp32_send(e, e->req);
    esp32_read_until(e, "CLOSED", resp, resp_size, 10000000);

    esp32_at(e, "AT+CIPCLOSE", e->tmp, sizeof(e->tmp));
    return (resp != NULL) && (strstr(resp, "200") != NULL);
}

/* 查询本机(STA)的 IP：发 AT+CIFSR，从 +CIFSR:STAIP,"x.x.x.x" 里抠出 IP 存入 ip。
   成功且非空返回 true。 */
bool esp32_get_ip(esp32_t *e, char *ip, uint16_t size)
{
    if(ip == NULL || size == 0) return false;
    ip[0] = '\0';
    if(!esp32_at(e, "AT+CIFSR", e->tmp, sizeof(e->tmp))) return false;
    const char *p = strstr(e->tmp, "STAIP,\"");
    if(p == NULL) return false;
    p += 7;                                   /* 跳过  STAIP,"  共7字符 */
    uint16_t i = 0;
    while(*p && *p != '"' && i < size - 1) ip[i++] = *p++;
    ip[i] = '\0';
    return i > 0;
}

/* SNTP 月份名 → 数字(1~12) */
static uint8_t sntp_month(const char *m)
{
    static const char names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    for(uint8_t i = 0; i < 12; i++)
        if(m[0]==names[i*3] && m[1]==names[i*3+1] && m[2]==names[i*3+2]) return i+1;
    return 0;
}

/* 配置 SNTP 对时：tz=时区小时(北京=8)，server 如 "ntp.aliyun.com"。成功 true。 */
bool esp32_sntp_cfg(esp32_t *e, int tz, const char *server)
{
    snprintf(e->cmd, sizeof(e->cmd), "AT+CIPSNTPCFG=1,%d,\"%s\"", tz, server);
    return esp32_at(e, e->cmd, e->tmp, sizeof(e->tmp));
}

/* 查询 SNTP 当前时间并解析进 dt。回复形如 "+CIPSNTPTIME:Tue Nov  3 14:43:33 2026"。
   未同步(年份 1970)返回 false。 */
bool esp32_sntp_read(esp32_t *e, DateTime *dt)
{
    if(dt == NULL) return false;
    if(!esp32_at(e, "AT+CIPSNTPTIME?", e->tmp, sizeof(e->tmp))) return false;
    const char *p = strstr(e->tmp, "+CIPSNTPTIME:");
    if(p == NULL) return false;
    p += 13;                                  /* 跳过前缀 → "Tue Nov  3 14:43:33 2026" */

    dt->weekday[0]=p[0]; dt->weekday[1]=p[1]; dt->weekday[2]=p[2]; dt->weekday[3]='\0';
    dt->month = sntp_month(p + 4);

    const char *q = p + 7;                    /* 月份名后跳空格读"日" */
    while(*q == ' ') q++;
    int day = 0;
    while(*q >= '0' && *q <= '9') { day = day*10 + (*q - '0'); q++; }
    dt->day = (uint8_t)day;

    const char *c1 = strchr(q, ':');          /* 时:分:秒 */
    if(c1 == NULL) return false;
    dt->hour = (uint8_t)((c1[-2]-'0')*10 + (c1[-1]-'0'));
    dt->min  = (uint8_t)((c1[1]-'0')*10 + (c1[2]-'0'));
    const char *c2 = strchr(c1 + 1, ':');
    if(c2 == NULL) return false;
    dt->sec  = (uint8_t)((c2[1]-'0')*10 + (c2[2]-'0'));

    const char *y = c2 + 3;                   /* 秒后跳空格读 4 位"年" */
    while(*y == ' ') y++;
    int year = 0, cnt = 0;
    while(*y >= '0' && *y <= '9' && cnt < 4) { year = year*10 + (*y - '0'); y++; cnt++; }
    dt->year = (uint16_t)year;
    return dt->year > 2000;                   /* 已同步(非 1970)才算成功 */
}

/* USART2 总线空闲中断：一串字节到齐(IDLE)后，把 DMA 缓冲新到部分搬进软件环。
   环满丢新字节(不覆盖未读数据)；读取侧在主循环只消费环，与硬件天然隔离。 */
void USART2_IRQHandler(void)
{
    esp32_t *e = &esp32_modem;
    if(USART_GetITStatus(e->usart, USART_IT_IDLE) != RESET)
    {
        uint16_t pos, last, next;
        (void)e->usart->SR;                   /* 清 IDLE：先读 SR 再读 DR */
        (void)e->usart->DR;

        pos  = (uint16_t)((ESP32_RX_DMA_SIZE - DMA_GetCurrDataCounter(DMA1_Stream5)) % ESP32_RX_DMA_SIZE);
        last = e->rx_dma_pos;
        while(last != pos)
        {
            next = (uint16_t)((e->rx_head + 1) % ESP32_RX_RING_SIZE);
            if(next != e->rx_tail)            /* 环未满才写入 */
            {
                e->rx_ring[e->rx_head] = (char)e->rx_dma[last];
                e->rx_head = next;
            }
            last = (uint16_t)((last + 1) % ESP32_RX_DMA_SIZE);
        }
        e->rx_dma_pos = pos;
    }
}
