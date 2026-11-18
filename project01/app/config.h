#ifndef __CONFIG_H__
#define __CONFIG_H__

/* ===== 全局配置：集中管理，换网/换城市/调布局只改这里 ===== */

/* --- WiFi 凭据 --- */
#define WIFI_SSID  "Yio"
#define WIFI_PWD   "LYYLYYhahaha"

/* --- 天气 API（心知，HTTP 明文）--- */
#define WX_HOST    "api.seniverse.com"
#define WX_PATH    "/v3/weather/now.json?key=ShtD5k30cyCu4uhiG&location=WTTDPCGXTWUS&language=en&unit=c"

/* --- NTP 对时 --- */
#define NTP_SERVER "ntp.aliyun.com"
#define NTP_TZ     8                 /* 东八区 */

/* --- 湿度采集（AHT20 挂 I2C1: PB6/PB7，每 N 秒读一次刷新湿度卡）--- */
#define HUM_READ_PERIOD_S 10

/* --- 主界面布局坐标（v5 boxed 包：图标=左上角；文本=文本块中心，居中绘制）--- */
/* 图标左上角（v5 csv 给定）*/
#define MP_WIFI_X    24
#define MP_WIFI_Y    13
#define MP_PERIOD_X  51
#define MP_PERIOD_Y  13
#define MP_WXICON_X  27
#define MP_WXICON_Y  123
#define MP_THERM_X   26
#define MP_THERM_Y   207
#define MP_HUMICON_X 133
#define MP_HUMICON_Y 207
/* 文本块中心（v5 text 矩形中心）与矩形宽（超宽自动降 16 号）*/
#define MP_DATE_CX    110
#define MP_DATE_CY    23
#define MP_DATE_W     62
#define MP_WEEKDAY_CX 179
#define MP_WEEKDAY_CY 23
#define MP_WEEKDAY_W  50
#define MP_TIME_CX    120
#define MP_TIME_CY    77
#define MP_CITY_CX    144
#define MP_CITY_CY    133
#define MP_CITY_W     122
#define MP_WXTEXT_CX  144
#define MP_WXTEXT_CY  158
#define MP_WXTEXT_W   122
#define MP_TEMP_CX    84
#define MP_TEMP_CY    218
#define MP_TEMP_W     50
#define MP_HUM_CX     191
#define MP_HUM_CY     218
#define MP_HUM_W      50
/* 更新区域（刷新前擦回防残影，v5 update_region）*/
#define MP_REG_STATUS_X 17
#define MP_REG_STATUS_Y 8
#define MP_REG_STATUS_W 206
#define MP_REG_STATUS_H 31
#define MP_REG_TIME_X   21
#define MP_REG_TIME_Y   42
#define MP_REG_TIME_W   198
#define MP_REG_TIME_H   70

/* --- 欢迎页标题位置（20号字13字=208px 居中）--- */
#define UI_TITLE_X 16
#define UI_TITLE_Y 140

#endif /* __CONFIG_H__ */
