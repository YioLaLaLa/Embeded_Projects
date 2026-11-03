#include "esp32.h"
#include "stm32f4xx.h"
#include <string.h>
#include <stdio.h>

/* ===== USART2 底层：PA2=TX(AF7), PA3=RX(AF7)，USART2 挂在 APB1 ===== */

/* 读一个字节，带循环计数超时(非精确毫秒)；收到返回 true，超时返回 false。 */
static bool esp32_read_byte(uint8_t *b, uint32_t timeout)
{
    while(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == RESET)
        if(--timeout == 0) return false;
    *b = (uint8_t)USART_ReceiveData(USART2);   /* 读 DR 兼清 RXNE */
    return true;
}

/* 持续读入 buf，直到出现 endmark / buf 满 / 每字节超时；返回已读长度并补 '\0'。
   endmark=NULL 时只按超时收。per_byte_to 是相邻两字节间允许的最大空转圈数。 */
static uint16_t esp32_read_until(const char *endmark, char *buf, uint16_t size, uint32_t per_byte_to)
{
    uint16_t n = 0;
    if(buf == NULL || size == 0) return 0;
    buf[0] = '\0';
    while(n < size - 1)
    {
        uint8_t b;
        if(!esp32_read_byte(&b, per_byte_to)) break;
        buf[n++] = (char)b;
        buf[n] = '\0';
        if(endmark != NULL && strstr(buf, endmark) != NULL) break;
    }
    return n;
}

void esp32_init(void)
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
}

void esp32_send(const char *s)
{
    while(*s)
    {
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, (uint8_t)*s++);
    }
}

/* 内部：发 AT 指令(补 \r\n)，读到出现 OK 或 ERROR 为止。含 OK 且无 ERROR 返回 true。
   per_byte_to 越大，越能容忍慢指令(如 CWJAP 联网要几秒)。 */
static bool esp32_at_to(const char *cmd, char *resp, uint16_t resp_size, uint32_t per_byte_to)
{
    /* 清空残留接收数据 */
    while(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET)
        USART_ReceiveData(USART2);

    esp32_send(cmd);
    esp32_send("\r\n");

    if(resp == NULL || resp_size < 2)
    {
        uint8_t b; esp32_read_byte(&b, per_byte_to);   /* 不关心内容，简单消费一下 */
        return false;
    }
    /* 逐字节收，直到出现 OK / ERROR */
    uint16_t n = 0; uint8_t b;
    resp[0] = '\0';
    while(n < resp_size - 1)
    {
        if(!esp32_read_byte(&b, per_byte_to)) break;
        resp[n++] = (char)b;
        resp[n] = '\0';
        if(strstr(resp, "OK") != NULL || strstr(resp, "ERROR") != NULL) break;
    }
    return (strstr(resp, "OK") != NULL) && (strstr(resp, "ERROR") == NULL);
}

bool esp32_at(const char *cmd, char *resp, uint16_t resp_size)
{
    return esp32_at_to(cmd, resp, resp_size, 3000000);   /* 普通快指令 ~1s 超时 */
}

/* 连 WiFi：设 STA 模式 + AT+CWJAP。成功返回 true，过程回复存 resp。
   注意：CWJAP 联网可能要几秒，故用较大超时。 */
bool esp32_connect_wifi(const char *ssid, const char *pwd, char *resp, uint16_t resp_size)
{
    static char cmd[128];
    static char tmp[192];

    esp32_at_to("AT+CWMODE=1", tmp, sizeof(tmp), 3000000);       /* STA 模式 */
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
    bool ok = esp32_at_to(cmd, tmp, sizeof(tmp), 120000000);     /* 联网，超时要大 */
    if(resp != NULL && resp_size > 0) snprintf(resp, resp_size, "%s", tmp);
    return ok;
}

/* 通过 TCP 向 host:80 发 HTTP GET(path)，把服务器原始回复(含HTTP头)收进 resp。
   回复里出现 "200" 视为成功。resp_size 建议 ≥600。
   仅适用 HTTP 明文(MINI 固件不支持 SSL/HTTPS)。 */
bool esp32_http_get(const char *host, const char *path, char *resp, uint16_t resp_size)
{
    static char cmd[96];
    static char req[256];
    static char tmp[64];

    /* 组装 HTTP 请求报文 */
    int reqlen = snprintf(req, sizeof(req),
                          "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
                          path, host);
    if(reqlen <= 0 || reqlen >= (int)sizeof(req)) return false;

    /* 关掉可能残留的旧连接(失败无妨) */
    esp32_at("AT+CIPCLOSE", tmp, sizeof(tmp));

    /* 建 TCP 连接到 80 端口 */
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",80", host);
    if(!esp32_at_to(cmd, tmp, sizeof(tmp), 30000000))
    {
        if(resp) snprintf(resp, resp_size, "CIPSTART fail: %s", tmp);
        return false;
    }

    /* 声明发送字节数，等待 ">" 提示符 */
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d", reqlen);
    esp32_send(cmd);
    esp32_send("\r\n");
    esp32_read_until(">", tmp, sizeof(tmp), 5000000);

    /* 发原始 HTTP 报文，再收完整回复(直到 CLOSED) */
    esp32_send(req);
    esp32_read_until("CLOSED", resp, resp_size, 40000000);

    esp32_at("AT+CIPCLOSE", tmp, sizeof(tmp));
    return (resp != NULL) && (strstr(resp, "200") != NULL);
}

/* 查询本机(STA)的 IP：发 AT+CIFSR，从 +CIFSR:STAIP,"x.x.x.x" 里抠出 IP 存入 ip。
   成功且非空返回 true。 */
bool esp32_get_ip(char *ip, uint16_t size)
{
    static char tmp[160];
    if(ip == NULL || size == 0) return false;
    ip[0] = '\0';
    if(!esp32_at("AT+CIFSR", tmp, sizeof(tmp))) return false;
    const char *p = strstr(tmp, "STAIP,\"");
    if(p == NULL) return false;
    p += 7;                                   /* 跳过  STAIP,"  共7字符 */
    uint16_t i = 0;
    while(*p && *p != '"' && i < size - 1) ip[i++] = *p++;
    ip[i] = '\0';
    return i > 0;
}

/* 极简 JSON 取值：从 json 中找 "key":"value"，把 value 拷进 out（仅限字符串值）。
   找到且非空返回 true。够用于取天气的 text/temperature 等字段。 */
bool json_get_str(const char *json, const char *key, char *out, uint16_t out_size)
{
    if(json == NULL || key == NULL || out == NULL || out_size == 0) return false;
    char pat[40];
    snprintf(pat, sizeof(pat), "\"%s\":\"", key);        /* 构造 "key":" */
    const char *p = strstr(json, pat);
    if(p == NULL) return false;
    p += strlen(pat);
    uint16_t i = 0;
    while(*p && *p != '"' && i < out_size - 1) out[i++] = *p++;
    out[i] = '\0';
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
bool esp32_sntp_cfg(int tz, const char *server)
{
    static char cmd[96], tmp[96];
    snprintf(cmd, sizeof(cmd), "AT+CIPSNTPCFG=1,%d,\"%s\"", tz, server);
    return esp32_at(cmd, tmp, sizeof(tmp));
}

/* 查询 SNTP 当前时间并解析进 dt。回复形如 "+CIPSNTPTIME:Tue Nov  3 14:43:33 2026"。
   未同步(年份 1970)返回 false。 */
bool esp32_sntp_read(DateTime *dt)
{
    static char tmp[96];
    if(dt == NULL) return false;
    if(!esp32_at("AT+CIPSNTPTIME?", tmp, sizeof(tmp))) return false;
    const char *p = strstr(tmp, "+CIPSNTPTIME:");
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
