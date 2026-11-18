#ifndef __RTC_H__
#define __RTC_H__

#include "stm32f4xx.h"
#include "stdint.h"
#include "stdbool.h"
#include "datetime.h"

/* STM32F407 内部 RTC。上电后 rtc_init 选时钟源(优先 LSE 32.768kHz，无晶振则回退 LSI)，
   由 SNTP 对时一次写入(rtc_set)，之后 RTC 自主走时，主循环 rtc_get 每秒刷新显示。 */

/* RTC 日期时间统一用公共类型 DateTime（见 datetime.h）*/

/* 初始化 RTC：优先 LSE，失败回退 LSI，配好预分频得到 1Hz。
   src 非空时回写时钟源(0=LSE, 1=LSI)。成功返回 true。 */
bool rtc_init(uint8_t *src);

/* 写入日期时间(用于 SNTP 对时) */
void rtc_set(const DateTime *t);

/* 读出当前日期时间 */
void rtc_get(DateTime *t);

/* 由年月日算星期缩写("Sun".."Sat")，返回指向静态字符串的指针 */
const char *rtc_weekday(uint16_t year, uint8_t month, uint8_t day);

#endif /* __RTC_H__ */
