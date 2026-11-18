#include "weather.h"
#include "config.h"
#include "esp32.h"
#include "json.h"
#include <string.h>

/* 取天气并解析进 w。HTTP 整包约 700+ 字节，缓冲放 static 避免占栈。
   先填占位默认值，即使取数失败也有内容可显示。 */
bool weather_fetch(Weather *w)
{
	static char buf[1200];
	if(w == NULL) return false;

	strcpy(w->city, "--"); strcpy(w->text, "N/A");
	strcpy(w->code, "-");  strcpy(w->temp, "--");

	if(!esp32_http_get(&esp32_modem, WX_HOST, WX_PATH, buf, sizeof(buf))) return false;

	json_get_str(buf, "name",        w->city, sizeof(w->city));
	json_get_str(buf, "text",        w->text, sizeof(w->text));
	json_get_str(buf, "code",        w->code, sizeof(w->code));
	json_get_str(buf, "temperature", w->temp, sizeof(w->temp));
	return true;
}
