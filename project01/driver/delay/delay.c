#include "delay.h"
#include "stm32f4xx.h"

static uint32_t fac_us = 0;   // 每 1us 对应的 SysTick 计数值
static uint32_t fac_ms = 0;   // 每 1ms 对应的 SysTick 计数值

/*
SysTick 延时初始化：时钟源选 HCLK（= SystemCoreClock，F407 默认 168MHz），
不用中断，纯轮询 COUNTFLAG。先刷新 SystemCoreClock 再算分频系数，避免依赖默认值。
*/
void Delay_Init(void)
{
	SystemCoreClockUpdate();                        // 从 RCC 寄存器刷新实际 HCLK
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;     // 关计数中断，纯轮询
	SysTick->CTRL |=  SysTick_CTRL_CLKSOURCE_Msk;   // 时钟源 = HCLK（不分频）
	fac_us = SystemCoreClock / 1000000U;            // 168MHz → 每 us 计 168 次
	fac_ms = fac_us * 1000U;                        // 每 ms 计 168000 次
}

/* 单段延时：ticks 必须 ≤ 0xFFFFFF（SysTick 是 24 位计数器） */
static void Delay_Tick(uint32_t ticks)
{
	SysTick->LOAD = ticks & SysTick_LOAD_RELOAD_Msk;    // 重装值
	SysTick->VAL  = 0U;                                 // 清空当前值（同时清 COUNTFLAG）
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;           // 启动计数
	while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)); // 计到 0 置位 COUNTFLAG（读 CTRL 后自动清）
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;          // 停止
	SysTick->VAL  = 0U;
}

/* 按总 tick 数延时，超过 24 位上限则自动分段 */
static void Delay_Ticks(uint32_t ticks)
{
	while(ticks > SysTick_LOAD_RELOAD_Msk)
	{
		Delay_Tick(SysTick_LOAD_RELOAD_Msk);
		ticks -= SysTick_LOAD_RELOAD_Msk;
	}
	if(ticks) Delay_Tick(ticks);
}

void Delay_us(uint32_t nus)
{
	Delay_Ticks(nus * fac_us);
}

void Delay_ms(uint32_t nms)
{
	Delay_Ticks(nms * fac_ms);
}
