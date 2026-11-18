#ifndef __DATETIME_H__
#define __DATETIME_H__

#include "stdint.h"

/* 公共日期时间类型：SNTP 对时、RTC 走时、UI 显示共用，
   避免 esp32/rtc 各定义一份重复结构体和相互转换。 */
typedef struct {
	uint16_t year;                 /* 4 位年，如 2026 */
	uint8_t  month, day, hour, min, sec;
	char     weekday[4];           /* 星期缩写，如 "Tue"；可为空 */
} DateTime;

#endif /* __DATETIME_H__ */
