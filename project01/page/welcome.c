#include "page.h"
#include "config.h"
#include "st7789.h"
#include "font.h"

/* 欢迎页对象：纯代码绘制（黑底+标题），不用全屏图以省 Flash；静态页无 tick */
static void welcome_enter(page_t *self)
{
	(void)self;
	st7789_fill(ST7789_BLACK);
	font_show(&font20, UI_TITLE_X, UI_TITLE_Y, "Weather Clock", ST7789_CYAN, ST7789_BLACK);
	font_show(&font16, UI_TITLE_X, UI_TITLE_Y + 30, "Loading...", ST7789_WHITE, ST7789_BLACK);
}

static page_t s_welcome_page = { welcome_enter, NULL };

page_t *page_welcome(void)
{
	return &s_welcome_page;
}
