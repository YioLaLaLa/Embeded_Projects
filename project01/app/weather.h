#ifndef __WEATHER_H__
#define __WEATHER_H__

#include "stdbool.h"

/* 天气数据模型（从心知 API 的 JSON 解析而来）*/
typedef struct {
	char city[32];   /* 城市名 location.name */
	char text[32];   /* 天气现象 now.text，如 Cloudy */
	char code[8];    /* 天气代码 now.code，将来映射图标 */
	char temp[8];    /* 温度 now.temperature */
} Weather;

/* 取天气并解析：内部发 HTTP GET + JSON 解析。成功返回 true；
   失败时 w 仍被填为占位默认值("--"/"N/A")，可直接用于显示。 */
bool weather_fetch(Weather *w);

#endif /* __WEATHER_H__ */
