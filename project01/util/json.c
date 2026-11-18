#include "json.h"
#include <string.h>
#include <stdio.h>

/* 极简 JSON 取值：从 json 中找 "key":"value"，把 value 拷进 out（仅限字符串值）。
   找到且非空返回 true。够用于取天气的 text/temperature 等字段。 */
bool json_get_str(const char *json, const char *key, char *out, uint16_t out_size)
{
	if(json == NULL || key == NULL || out == NULL || out_size == 0) return false;
	char pat[40];
	snprintf(pat, sizeof(pat), "\"%s\":\"", key);        /* 构造 "key":" */
	const char *p = strstr(json, pat);
	if(p == NULL) return false;
	p += strlen(pat);
	uint16_t i = 0;
	while(*p && *p != '"' && i < out_size - 1) out[i++] = *p++;
	out[i] = '\0';
	return i > 0;
}
