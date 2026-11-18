#include "rtc.h"

/* ===== 星期计算：Sakamoto 算法，返回 0=周日 .. 6=周六 ===== */
static uint8_t weekday_index(uint16_t y, uint8_t m, uint8_t d)
{
	static const uint8_t t[12] = {0,3,2,5,0,3,5,1,4,6,2,4};
	if(m < 3) y--;
	return (uint8_t)((y + y/4 - y/100 + y/400 + t[m-1] + d) % 7);
}

const char *rtc_weekday(uint16_t year, uint8_t month, uint8_t day)
{
	static const char *names[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	return names[weekday_index(year, month, day)];
}

/* ===== 初始化 =====
   时钟源优先 LSE(32.768kHz 外部晶振，走时精准)；板子没焊 LSE 晶振时其就绪标志一直不置位，
   回退到内部 LSI(~32kHz，略有偏差但能走)。两种源分别配预分频把计数降到 1Hz。 */
bool rtc_init(uint8_t *src)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	PWR_BackupAccessCmd(ENABLE);            /* 打开后备域写保护，才能配 RTC */

	uint32_t asyn, syn;
	uint8_t  source;
	uint32_t to;

	/* 尝试 LSE */
	RCC_LSEConfig(RCC_LSE_ON);
	to = 0x500000;
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET && --to);
	if(RCC_GetFlagStatus(RCC_FLAG_LSERDY) != RESET) {
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
		asyn = 127; syn = 255;              /* (127+1)*(255+1)=32768 → 1Hz */
		source = 0;
	} else {
		RCC_LSICmd(ENABLE);                 /* LSE 没起来，回退 LSI */
		to = 0x500000;
		while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET && --to);
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
		asyn = 124; syn = 255;              /* (124+1)*(255+1)=32000 → ~1Hz */
		source = 1;
	}
	if(src) *src = source;

	RCC_RTCCLKCmd(ENABLE);                  /* 使能 RTC 时钟 */
	if(RTC_WaitForSynchro() == ERROR) return false;

	RTC_InitTypeDef r;
	r.RTC_HourFormat   = RTC_HourFormat_24;
	r.RTC_AsynchPrediv = asyn;
	r.RTC_SynchPrediv  = syn;
	if(RTC_Init(&r) == ERROR) return false;

	/* 给个默认初值(未对时前读到 2000-01-01 00:00:00)，用 BIN 格式免 BCD 转换 */
	RTC_TimeTypeDef t; RTC_DateTypeDef d;
	t.RTC_Hours = 0; t.RTC_Minutes = 0; t.RTC_Seconds = 0; t.RTC_H12 = RTC_H12_AM;
	d.RTC_Year = 0; d.RTC_Month = 1; d.RTC_Date = 1; d.RTC_WeekDay = RTC_Weekday_Saturday;
	RTC_SetTime(RTC_Format_BIN, &t);
	RTC_SetDate(RTC_Format_BIN, &d);
	return true;
}

/* ===== 写入日期时间(先时间后日期，ST 规定顺序) ===== */
void rtc_set(const DateTime *tm)
{
	RTC_TimeTypeDef t; RTC_DateTypeDef d;
	t.RTC_Hours = tm->hour; t.RTC_Minutes = tm->min; t.RTC_Seconds = tm->sec; t.RTC_H12 = RTC_H12_AM;
	d.RTC_Year  = (uint8_t)(tm->year - 2000);   /* RTC 年寄存器只存 0..99，相对 2000 */
	d.RTC_Month = tm->month; d.RTC_Date = tm->day;
	/* 内部 weekday: 1=周一..7=周日；weekday_index 给 0=周日..6=周六 */
	uint8_t wi = weekday_index(tm->year, tm->month, tm->day);
	d.RTC_WeekDay = (wi == 0) ? 7 : wi;

	RTC_SetTime(RTC_Format_BIN, &t);
	RTC_SetDate(RTC_Format_BIN, &d);

	RTC_ClearFlag(RTC_FLAG_RSF);                /* 清影子寄存器标志 */
	while(RTC_GetFlagStatus(RTC_FLAG_RSF) == RESET);   /* 等新值同步到影子寄存器 */
}

/* ===== 读出当前日期时间(先读时间后读日期，解锁影子寄存器) ===== */
void rtc_get(DateTime *tm)
{
	RTC_TimeTypeDef t; RTC_DateTypeDef d;
	RTC_GetTime(RTC_Format_BIN, &t);
	RTC_GetDate(RTC_Format_BIN, &d);
	tm->hour = t.RTC_Hours; tm->min = t.RTC_Minutes; tm->sec = t.RTC_Seconds;
	tm->year = (uint16_t)(d.RTC_Year + 2000);
	tm->month = d.RTC_Month; tm->day = d.RTC_Date;
}
