#include "aht20.h"
#include "i2c.h"
#include "delay.h"
#include "stm32f4xx.h"
#include "stdbool.h"

#define AHT20_ADDR        0x70   // 7 位地址 0x38 左移 1 位（8 位写地址）；读地址 0x71 由 i2c_read 内部处理
#define AHT20_STATUS_BUSY 0x80   // 状态字节 bit7：1 = 测量中（忙）
#define AHT20_STATUS_CAL  0x08   // 状态字节 bit3：1 = 已校准

/* 读状态字节：AHT20 每次读事务返回的第一个字节即状态 */
bool AHT20_ReadStatus(uint8_t *status)
{
	return i2c_read(&i2c1_bus, AHT20_ADDR, status, 1);
}

/* 上电初始化：延时稳定 → 读状态 → 未校准则发初始化命令 0xBE 0x08 0x00 */
bool AHT20_Init(void)
{
	uint8_t status = 0;
	uint8_t cmd[3] = { 0xBE, 0x08, 0x00 };          // 初始化/校准命令

	Delay_ms(100);                                  // 上电稳定 ≥100ms（datasheet 要求）
	if(!AHT20_ReadStatus(&status)) return false;    // 初始化前先读一次状态
	if((status & AHT20_STATUS_CAL) == 0)            // bit3=0 → 未校准，需发初始化命令
	{
		if(!i2c_write(&i2c1_bus, AHT20_ADDR, cmd, 3)) return false;
		Delay_ms(10);                               // 初始化命令后 ≥10ms
	}
	return true;
}

/* 触发测量 → 等待完成 → 读 6 字节 → 拼 20bit 原始值 → 换算成 RH% 和 °C */
bool AHT20_Read(float *humidity, float *temperature)
{
	uint8_t cmd[3] = { 0xAC, 0x33, 0x00 };          // 触发测量命令
	uint8_t buf[6];                                 // buf[0]=状态，buf[1..5]=数据（忽略第 7 字节 CRC）
	uint32_t hum_raw, temp_raw;

	if(!i2c_write(&i2c1_bus, AHT20_ADDR, cmd, 3)) return false;   // 触发一次测量
	Delay_ms(80);                                   // 测量需 ≥75ms，等 80ms 稳妥

	if(!i2c_read(&i2c1_bus, AHT20_ADDR, buf, 6)) return false;    // 读状态 + 5 字节数据
	if(buf[0] & AHT20_STATUS_BUSY) return false;    // 仍在忙 → 本次数据无效

	/* 20 位湿度 = buf[1] buf[2] buf[3]高4位；20 位温度 = buf[3]低4位 buf[4] buf[5] */
	hum_raw  = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | ((uint32_t)buf[3] >> 4);
	temp_raw = ((uint32_t)(buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | (uint32_t)buf[5];

	*humidity    = (float)hum_raw  * 100.0f / 1048576.0f;         // RH% = raw / 2^20 × 100
	*temperature = (float)temp_raw * 200.0f / 1048576.0f - 50.0f; // T°C = raw / 2^20 × 200 − 50
	return true;
}
