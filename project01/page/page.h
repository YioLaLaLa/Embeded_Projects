#ifndef __PAGE_H__
#define __PAGE_H__

#include <stddef.h>   /* NULL */
#include "stdbool.h"
#include "weather.h"
#include "datetime.h"

/* 界面层：每个界面是一个页面对象（.c 内定义实例，此处只暴露接口与工厂函数）。
   切页 = 换 page_t 指针；enter 画静态部分，tick 周期刷新（静态页为 NULL）。
   坐标/颜色配置见 app/config.h。 */
typedef struct page page_t;
struct page {
	void (*enter)(page_t *self);                    /* 进入页：画静态部分 */
	void (*tick)(page_t *self, const DateTime *dt); /* 周期刷新，可为 NULL */
};

/* 欢迎页(开机加载页)：黑底 + 标题 */
page_t *page_welcome(void);

/* WiFi 连接中页：黑底 + 提示 */
page_t *page_wifi_connecting(void);

/* WiFi 连接失败页：96×96 错误图居中 */
page_t *page_wifi_error(void);

/* 主界面页：先注入天气数据与 WiFi 状态，返回页面对象，由调用方 enter/tick。
   动态部分 = 时间(52 号，冒号秒闪) + 状态栏(WiFi/昼夜/日期/星期)，变化才重绘。 */
page_t *page_main(const Weather *w, bool wifi_ok);

/* 主界面专属扩展方法：湿度卡刷新（AHT20 实测 RH%，0~100 超界限幅，先擦后画）*/
void page_main_set_humidity(int rh);

#endif /* __PAGE_H__ */
