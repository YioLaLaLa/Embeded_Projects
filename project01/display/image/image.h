#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "stdint.h"

/* ===== 图片资源：RGB565 字节流(每像素2字节，高字节在前) =====
   数据定义在各自的 .c 里，这里只做 extern 声明 + 尺寸宏。
   用 st7789_draw_rgb(x, y, W, H, 数组名) 绘制。 */

/* WiFi 错误界面 96×96（数据在 wifi_error_page.c）*/
#define IMG_WIFI_ERR_W  96
#define IMG_WIFI_ERR_H  96
extern const unsigned char gImage_wifi_error_page[];

/* ===== 小图标（数据在 image/batch/，RGB565，数组名带尺寸后缀，v5）===== */
/* 状态栏图标 20×20 */
#define IMG_STATUS_WIFI_W   20
#define IMG_STATUS_WIFI_H   20
extern const unsigned char gImage_status_wifi_20x20[];
extern const unsigned char gImage_status_wifi_off_20x20[];
/* 昼夜 20×20 */
#define IMG_PERIOD_DAY_W    20
#define IMG_PERIOD_DAY_H    20
extern const unsigned char gImage_period_day_20x20[];
extern const unsigned char gImage_period_night_20x20[];
/* 温湿度图标 24×24 */
#define IMG_DATA_THERM_W    24
#define IMG_DATA_THERM_H    24
extern const unsigned char gImage_data_thermometer_24x24[];
#define IMG_DATA_HUM_W      24
#define IMG_DATA_HUM_H      24
extern const unsigned char gImage_data_humidity_24x24[];
/* 天气主图标 44×44（各天气图标统一尺寸）*/
#define IMG_WEATHER_W       44
#define IMG_WEATHER_H       44
extern const unsigned char gImage_weather_sunny_44x44[];
extern const unsigned char gImage_weather_partly_cloudy_44x44[];
extern const unsigned char gImage_weather_cloudy_44x44[];
extern const unsigned char gImage_weather_light_rain_44x44[];
extern const unsigned char gImage_weather_heavy_rain_44x44[];
extern const unsigned char gImage_weather_snow_44x44[];
extern const unsigned char gImage_weather_fog_44x44[];
extern const unsigned char gImage_weather_thunderstorm_44x44[];

/* 主界面背景 240×320 全屏（数据在 main_bg.c）*/
#define IMG_MAIN_W  240
#define IMG_MAIN_H  320
extern const unsigned char gImage_main_bg[];

#endif /* __IMAGE_H__ */
