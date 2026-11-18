#include "page.h"
#include "st7789.h"
#include "image.h"

/* WiFi 连接失败页对象：96×96 错误图，居中显示（图片数据在 display/image/wifi_error_page.c）*/
static void wifi_error_enter(page_t *self)
{
	(void)self;
	st7789_fill(ST7789_BLACK);          /* 擦掉欢迎页/上一界面背景 */
	st7789_draw_rgb(72, 112, IMG_WIFI_ERR_W, IMG_WIFI_ERR_H, gImage_wifi_error_page);
}

static page_t s_wifi_error_page = { wifi_error_enter, NULL };

page_t *page_wifi_error(void)
{
	return &s_wifi_error_page;
}
