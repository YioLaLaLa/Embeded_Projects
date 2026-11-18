#ifndef __JSON_H__
#define __JSON_H__

#include "stdint.h"
#include "stdbool.h"

/* 极简 JSON 取值：从 json 中找 "key":"value"，把 value 拷进 out（仅限字符串值）。
   找到且非空返回 true。够用于取天气的 name/text/temperature 等字段。
   通用工具，与具体外设无关。 */
bool json_get_str(const char *json, const char *key, char *out, uint16_t out_size);

#endif /* __JSON_H__ */
