#include <stdint.h>
#include <string.h>
#include "delay.h"
#include "usart.h"
#include "esp32.h"
#include "sensor.h"
#include "aht20.h"
#include "rtc.h"
#include "datetime.h"
#include "config.h"
#include "weather.h"
#include "page.h"
#include "board.h"

/* main 只负责：板级初始化 → 采集一次(联网/天气/对时) → 进主循环刷新。
   外设初始化下沉到 board，界面面向 page_t 对象，modem 面向 esp32_modem 实例，
   传感器面向 sensor_t 接口，配置集中在 config.h。 */
int main(void)
{
	page_t *pg;

	board_lowlevel_init();            /* 延时 + 调试串口，之后才能打日志 */
	board_init();                     /* LCD / ESP32 串口 / I2C+AHT20 / RTC */

	pg = page_welcome();              /* 开机欢迎页，联网/取天气期间一直显示 */
	pg->enter(pg);

	static char buf[192];             /* 连 WiFi 的过程回复用，够大即可 */
	Weather  w;
	DateTime dt;
	char ip[24] = "-";
	uint8_t last_hum_sec = 0xFF;      /* 上次读 AHT20 的秒，防同一秒重读 */

	/* --- 连 WiFi + 查 IP --- */
	USART1_SendString("[WIFI] connecting...\r\n");
	pg = page_wifi_connecting();      /* 切页 = 换指针 */
	pg->enter(pg);
	bool wifi_ok = esp32_connect_wifi(&esp32_modem, WIFI_SSID, WIFI_PWD, buf, sizeof(buf));
	if(wifi_ok) {
		USART1_SendString("[WIFI] connected\r\n");
		if(esp32_get_ip(&esp32_modem, ip, sizeof(ip))) {
			USART1_SendString("[WIFI] IP = ");
			USART1_SendString(ip);
			USART1_SendString("\r\n");
		}
	} else {
		USART1_SendString("[WIFI] FAIL\r\n");
		pg = page_wifi_error();       /* WiFi 失败，切错误页 */
		pg->enter(pg);
		Delay_ms(3000);               /* 停留 3 秒让用户看清 */
	}

	/* --- 取天气(内部 HTTP GET + JSON 解析) --- */
	USART1_SendString("[HTTP] GET weather...\r\n");
	if(weather_fetch(&w)) {
		USART1_SendString("[WX] ");
		USART1_SendString(w.city); USART1_SendString(" ");
		USART1_SendString(w.text); USART1_SendString(" ");
		USART1_SendString(w.temp); USART1_SendString("C\r\n");
	} else {
		USART1_SendString("[HTTP] FAIL\r\n");
	}

	/* --- SNTP 对时 → 写入 RTC(RTC 已在 board_init 启动，自主走时) --- */
	memset(&dt, 0, sizeof(dt));
	esp32_sntp_cfg(&esp32_modem, NTP_TZ, NTP_SERVER);
	Delay_ms(4000);                   /* 等 SNTP 后台同步几秒 */
	if(!esp32_sntp_read(&esp32_modem, &dt))
		USART1_SendString("[TIME] SNTP fail/not synced\r\n");
	if(dt.year > 2000) {
		rtc_set(&dt);                 /* SNTP 结果直接写 RTC，DateTime 统一无需转换 */
		USART1_SendString("[RTC] synced from SNTP\r\n");
	}

	/* --- 切到时钟页，主循环周期驱动 tick --- */
	pg = page_main(&w, wifi_ok);      /* 注入天气/WiFi 状态，拿回页面对象 */
	pg->enter(pg);
	while(1)
	{
		rtc_get(&dt);
		pg->tick(pg, &dt);              /* 分/时/天/秒变化才重绘（秒=冒号闪烁）*/

		/* 每 HUM_READ_PERIOD_S 秒经 sensor_t 接口读一次，刷新湿度卡；失败保留上次值 */
		if(dt.sec != last_hum_sec && dt.sec % HUM_READ_PERIOD_S == 0) {
			float rh, tc;
			last_hum_sec = dt.sec;
			if(aht20_sensor.read(&rh, &tc))
				page_main_set_humidity((int)(rh + 0.5f));
		}
		Delay_ms(50);                 /* 降低轮询频率，够捕捉秒变化又不空转 */
	}
}
