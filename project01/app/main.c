#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"
#include "delay.h"
#include "usart.h"
#include "st7789.h"
#include "font.h"
#include "image.h"
#include "esp32.h"

/* ===== WiFi / 天气 / NTP 配置 ===== */
#define WIFI_SSID  "Yio"
#define WIFI_PWD   "LYYLYYhahaha"
#define WX_HOST    "api.seniverse.com"
#define WX_PATH    "/v3/weather/now.json?key=ShtD5k30cyCu4uhiG&location=WTTDPCGXTWUS&language=en&unit=c"
#define NTP_SERVER "ntp.aliyun.com"
#define NTP_TZ     8            /* 东八区 */

/* 解析出来的天气数据 */
typedef struct {
	char city[32];   /* 城市名 location.name */
	char text[32];   /* 天气现象 now.text */
	char code[8];    /* 天气代码 now.code，将来映射图标 */
	char temp[8];    /* 温度 now.temperature */
} Weather;

int main(void)
{
	Delay_Init();
	USART1_Init();
	USART1_SendString("\r\n===== Weather Clock =====\r\n");

	st7789_init();
	st7789_fill(ST7789_BLACK);

	esp32_init();
	static char buf[1200];       /* 大缓冲放 static，避免占用栈 */
	Weather  w;
	DateTime dt;
	char ip[24] = "-";

	/* --- 连 WiFi + 查 IP --- */
	USART1_SendString("[WIFI] connecting...\r\n");
	if(esp32_connect_wifi(WIFI_SSID, WIFI_PWD, buf, sizeof(buf))) {
		USART1_SendString("[WIFI] connected\r\n");
		if(esp32_get_ip(ip, sizeof(ip))) {
			USART1_SendString("[WIFI] IP = ");
			USART1_SendString(ip);
			USART1_SendString("\r\n");
		}
	} else {
		USART1_SendString("[WIFI] FAIL\r\n");
	}

	/* --- 取天气，解析进 w --- */
	strcpy(w.city,"--"); strcpy(w.text,"N/A"); strcpy(w.code,"-"); strcpy(w.temp,"--");
	USART1_SendString("[HTTP] GET weather...\r\n");
	if(esp32_http_get(WX_HOST, WX_PATH, buf, sizeof(buf))) {
		json_get_str(buf, "name",        w.city, sizeof(w.city));
		json_get_str(buf, "text",        w.text, sizeof(w.text));
		json_get_str(buf, "code",        w.code, sizeof(w.code));
		json_get_str(buf, "temperature", w.temp, sizeof(w.temp));
	} else {
		USART1_SendString("[HTTP] FAIL\r\n");
	}

	/* --- SNTP 对时，解析进 dt --- */
	memset(&dt, 0, sizeof(dt));
	esp32_sntp_cfg(NTP_TZ, NTP_SERVER);
	Delay_ms(4000);                 /* 等 SNTP 后台同步几秒 */
	if(!esp32_sntp_read(&dt))
		USART1_SendString("[TIME] SNTP fail/not synced\r\n");

	/* --- 串口打印汇总 --- */
	USART1_SendString("[WX] ");
	USART1_SendString(w.city); USART1_SendString(" ");
	USART1_SendString(w.text); USART1_SendString(" ");
	USART1_SendString(w.temp); USART1_SendString("C\r\n");
	snprintf(buf, sizeof(buf), "[TIME] %04d-%02d-%02d %s %02d:%02d:%02d\r\n",
	         dt.year, dt.month, dt.day, dt.weekday, dt.hour, dt.min, dt.sec);
	USART1_SendString(buf);

	/* --- LCD 显示：日期 / 时间 / 城市天气 / 温度 --- */
	char line[48];
	snprintf(line, sizeof(line), "%04d-%02d-%02d %s", dt.year, dt.month, dt.day, dt.weekday);
	font_show_string(10, 40, line, ST7789_WHITE,  ST7789_BLACK, 16);
	snprintf(line, sizeof(line), "%02d:%02d:%02d", dt.hour, dt.min, dt.sec);
	font_show_string(10, 80, line, ST7789_GREEN,  ST7789_BLACK, 16);
	snprintf(line, sizeof(line), "%s  %s", w.city, w.text);
	font_show_string(10, 140, line, ST7789_CYAN,   ST7789_BLACK, 16);
	snprintf(line, sizeof(line), "%s C", w.temp);
	font_show_string(10, 180, line, ST7789_YELLOW, ST7789_BLACK, 16);

	while(1) { }
}
