#include "page.h"
#include "config.h"
#include "st7789.h"
#include "font.h"

/* WiFi 连接中页对象：纯代码绘制（黑底+提示），不用全屏图以省 Flash */
static void wifi_connecting_enter(page_t *self)
{
	(void)self;
	st7789_fill(ST7789_BLACK);
	font_show(&font16, 48, 150, "WiFi connecting...", ST7789_YELLOW, ST7789_BLACK);
}

static page_t s_wifi_connecting_page = { wifi_connecting_enter, NULL };

page_t *page_wifi_connecting(void)
{
	return &s_wifi_connecting_page;
}
