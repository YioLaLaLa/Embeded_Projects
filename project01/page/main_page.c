#include "page.h"
#include "config.h"
#include "st7789.h"
#include "font.h"
#include "image.h"
#include "rtc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 主界面页对象：实例状态（天气数据、WiFi 标志、上次绘制值）全部收进结构体，
   不再用文件级 static 变量；vtable 见 page.h 的 page_t。 */
typedef struct {
	page_t   base;                 /* 接口须为首成员，page_t* 与本类型可互转 */
	Weather  w;                    /* enter 用的天气快照 */
	bool     wifi_ok;
	uint8_t  last_min, last_hour, last_day, last_sec;   /* 变化检测，0xFF=未画过 */
} main_page_t;

static void main_enter(page_t *self);
static void main_tick(page_t *self, const DateTime *dt);

static main_page_t s_main_page = { { main_enter, main_tick } };

/* 心知天气代码 → 天气图标（各天气图标统一 44×44）*/
static const unsigned char *wx_icon(const char *code)
{
	int c = atoi(code);
	if(c <= 1)  return gImage_weather_sunny_44x44;          /* 晴 */
	if(c <= 3)  return gImage_weather_partly_cloudy_44x44;  /* 多云 */
	if(c <= 5)  return gImage_weather_cloudy_44x44;         /* 阴 */
	if(c <= 9)  return gImage_weather_light_rain_44x44;     /* 阵雨 */
	if(c <= 13) return gImage_weather_heavy_rain_44x44;     /* 雨 */
	if(c <= 17) return gImage_weather_snow_44x44;           /* 雪 */
	if(c <= 21) return gImage_weather_fog_44x44;            /* 雾霾 */
	if(c <= 25) return gImage_weather_thunderstorm_44x44;   /* 雷暴 */
	return gImage_weather_fog_44x44;
}

/* 居中透明文本：(cx,cy) 为文本块中心，按字数×字宽算左上角。
   max_w 非 0 且 20 号字超宽时自动降为 16 号，保证不出框。 */
static void draw_centered(uint16_t cx, uint16_t cy, const char *s, uint16_t fg,
                          const font_t *f, uint16_t max_w)
{
	int16_t x, y;
	if(max_w && f == &font20 && (uint16_t)strlen(s) * f->w > max_w) f = &font16;
	x = (int16_t)cx - (int16_t)strlen(s) * f->w / 2;
	y = (int16_t)cy - (int16_t)f->h / 2;
	if(x < 0) x = 0;
	if(y < 0) y = 0;
	font_show_transparent(f, (uint16_t)x, (uint16_t)y, s, fg);
}

/* 状态栏：WiFi 图标 + 昼夜图标 + 日期 + 星期。四者同在 REGION_STATUS 内，擦除后须整体重画 */
static void draw_status_bar(const main_page_t *p, const DateTime *dt)
{
	char line[16];
	st7789_draw_rgb_transparent(MP_WIFI_X, MP_WIFI_Y, IMG_STATUS_WIFI_W, IMG_STATUS_WIFI_H,
	                            p->wifi_ok ? gImage_status_wifi_20x20 : gImage_status_wifi_off_20x20, ST7789_BLACK);
	st7789_draw_rgb_transparent(MP_PERIOD_X, MP_PERIOD_Y, IMG_PERIOD_DAY_W, IMG_PERIOD_DAY_H,
	                            (dt->hour >= 6 && dt->hour < 18) ? gImage_period_day_20x20 : gImage_period_night_20x20, ST7789_BLACK);
	snprintf(line, sizeof(line), "%02d-%02d", dt->month, dt->day);
	draw_centered(MP_DATE_CX, MP_DATE_CY, line, ST7789_WHITE, &font20, MP_DATE_W);
	draw_centered(MP_WEEKDAY_CX, MP_WEEKDAY_CY, rtc_weekday(dt->year, dt->month, dt->day), ST7789_WHITE, &font20, MP_WEEKDAY_W);
}

/* enter：画静态部分（背景 + 天气区 + 温湿度卡，湿度先显占位），并复位变化检测 */
static void main_enter(page_t *self)
{
	main_page_t *p = (main_page_t *)self;
	char line[48];

	p->last_min = p->last_hour = p->last_day = p->last_sec = 0xFF;
	st7789_fill(ST7789_BLACK);
	st7789_draw_rgb(0, 0, IMG_MAIN_W, IMG_MAIN_H, gImage_main_bg);   /* 背景（底层，不透明）*/

	/* 天气区：图标 + 城市 + 天气描述（色键透明/框内居中）*/
	st7789_draw_rgb_transparent(MP_WXICON_X, MP_WXICON_Y, IMG_WEATHER_W, IMG_WEATHER_H, wx_icon(p->w.code), ST7789_BLACK);
	draw_centered(MP_CITY_CX, MP_CITY_CY, p->w.city, ST7789_BLACK, &font20, MP_CITY_W);
	draw_centered(MP_WXTEXT_CX, MP_WXTEXT_CY, p->w.text, ST7789_BLACK, &font20, MP_WXTEXT_W);

	/* 温湿卡：图标 + 数值（TEMP/HUM 标签已烤入背景图）*/
	st7789_draw_rgb_transparent(MP_THERM_X, MP_THERM_Y, IMG_DATA_THERM_W, IMG_DATA_THERM_H, gImage_data_thermometer_24x24, ST7789_BLACK);
	snprintf(line, sizeof(line), "%sC", p->w.temp);
	draw_centered(MP_TEMP_CX, MP_TEMP_CY, line, ST7789_BLACK, &font20, MP_TEMP_W);
	st7789_draw_rgb_transparent(MP_HUMICON_X, MP_HUMICON_Y, IMG_DATA_HUM_W, IMG_DATA_HUM_H, gImage_data_humidity_24x24, ST7789_BLACK);
	draw_centered(MP_HUM_CX, MP_HUM_CY, "--%", ST7789_BLACK, &font20, MP_HUM_W);  /* 占位，待 AHT20 实测刷新 */
}

/* tick：时间(52，冒号秒闪) + 状态栏；分/时/天/秒变化才重绘 */
static void main_tick(page_t *self, const DateTime *dt)
{
	main_page_t *p = (main_page_t *)self;
	char line[48];
	if(dt == NULL) return;

	if(dt->min != p->last_min) {
		p->last_min = dt->min;
		snprintf(line, sizeof(line), "%02d:%02d", dt->hour, dt->min);
		/* 透明文字不擦旧字形，重画前按 v5 指定 update region 擦回，防残影 */
		st7789_restore_bg(gImage_main_bg, IMG_MAIN_W, MP_REG_TIME_X, MP_REG_TIME_Y, MP_REG_TIME_W, MP_REG_TIME_H);
		draw_centered(MP_TIME_CX, MP_TIME_CY, line, ST7789_WHITE, &font52, 0);
	}
	if(dt->sec != p->last_sec) {
		uint16_t cx = MP_TIME_CX - 5 * font52.w / 2 + 2 * font52.w;
		uint16_t cy = MP_TIME_CY - font52.h / 2;
		p->last_sec = dt->sec;
		/* 冒号闪烁：偶数秒显示，奇数秒用背景擦回该字形区；
		   位置与 draw_centered 的 "HH:MM" 第 3 个字符对齐 */
		if(dt->sec & 1)
			st7789_restore_bg(gImage_main_bg, IMG_MAIN_W, cx, cy, font52.w, font52.h);
		else
			font_show_transparent(&font52, cx, cy, ":", ST7789_WHITE);
	}
	if(dt->hour != p->last_hour || dt->day != p->last_day) {
		p->last_hour = dt->hour;
		p->last_day = dt->day;
		/* 状态栏区域含 WiFi/昼夜/日期/星期，擦回后整体重画 */
		st7789_restore_bg(gImage_main_bg, IMG_MAIN_W, MP_REG_STATUS_X, MP_REG_STATUS_Y, MP_REG_STATUS_W, MP_REG_STATUS_H);
		draw_status_bar(p, dt);
	}
}

/* 工厂：注入数据 → 返回页面对象（enter/tick 由调用方驱动）*/
page_t *page_main(const Weather *w, bool wifi_ok)
{
	if(w != NULL) s_main_page.w = *w;
	s_main_page.wifi_ok = wifi_ok;
	return &s_main_page.base;
}

/* 主界面专属方法：湿度卡刷新；透明文字不擦旧字，先擦回文本矩形再重画 */
void page_main_set_humidity(int rh)
{
	char line[16];
	if(rh < 0)   rh = 0;
	if(rh > 100) rh = 100;
	snprintf(line, sizeof(line), "%d%%", rh);
	st7789_restore_bg(gImage_main_bg, IMG_MAIN_W,
	                  MP_HUM_CX - MP_HUM_W / 2, MP_HUM_CY - font20.h / 2, MP_HUM_W, font20.h);
	draw_centered(MP_HUM_CX, MP_HUM_CY, line, ST7789_BLACK, &font20, MP_HUM_W);
}
