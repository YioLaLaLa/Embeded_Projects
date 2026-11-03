#ifndef __ESP32_H__
#define __ESP32_H__

#include "stm32f4xx.h"
#include "stdint.h"
#include "stdbool.h"

/* ESP32-C3(AT 固件) 经 USART2 连接：
   STM32 PA2(TX) → ESP32 GPIO6(RX)；STM32 PA3(RX) ← ESP32 GPIO7(TX)；共地。
   波特率按你 ESP32 AT 固件实际值改这里(默认 115200)。 */
#define ESP32_BAUDRATE   115200

/* 初始化 USART2（连接 ESP32-C3 的那一路串口）*/
void esp32_init(void);

/* 向 ESP32 原样发送一个字符串（不自动加 \r\n）*/
void esp32_send(const char *s);

/* 发送一条 AT 指令(自动补 \r\n)，把回复收进 resp 并补 '\0'。
   回复中出现 "OK" 返回 true；出现 "ERROR" 或超时返回 false。
   resp_size 建议 ≥128。 */
bool esp32_at(const char *cmd, char *resp, uint16_t resp_size);

/* 连 WiFi：设 STA 模式 + AT+CWJAP。成功返回 true，过程回复存 resp(建议≥192)。 */
bool esp32_connect_wifi(const char *ssid, const char *pwd, char *resp, uint16_t resp_size);

/* 经 TCP 向 host:80 发 HTTP GET(path)，原始回复(含HTTP头)收进 resp(建议≥600)。
   仅 HTTP 明文(MINI 固件不支持 HTTPS)。回复含 "200" 返回 true。 */
bool esp32_http_get(const char *host, const char *path, char *resp, uint16_t resp_size);

/* 极简 JSON 取值：找 "key":"value"，把 value 拷进 out。成功返回 true。 */
bool json_get_str(const char *json, const char *key, char *out, uint16_t out_size);

/* 查询本机(STA) IP（发 AT+CIFSR 解析），存入 ip。成功返回 true。 */
bool esp32_get_ip(char *ip, uint16_t size);

/* SNTP 解析出的日期时间 */
typedef struct {
	uint16_t year;
	uint8_t  month, day, hour, min, sec;
	char     weekday[4];   /* 星期缩写，如 "Tue" */
} DateTime;

/* 配置 SNTP 对时：tz=时区(北京=8)，server 如 "ntp.aliyun.com"。成功 true。 */
bool esp32_sntp_cfg(int tz, const char *server);

/* 查询 SNTP 当前时间并解析进 dt(未同步/1970 返回 false)。成功 true。 */
bool esp32_sntp_read(DateTime *dt);

#endif /* __ESP32_H__ */
