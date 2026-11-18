#ifndef __AHT20_H__
#define __AHT20_H__

#include "sensor.h"

/*
AHT20 温湿度传感器（I2C，7 位地址 0x38）—— sensor_t 接口的一个实现。
依赖：driver/I2C（i2c_write/i2c_read）、driver/delay（Delay_ms）。
总线：i2c.c 里的 i2c1_bus（I2C1，PB6=SCL PB7=SDA）；换引脚改 i2c1_bus 即可。
应用层只经 aht20_sensor 接口访问，不直接调内部函数。
*/
extern const sensor_t aht20_sensor;

#endif
