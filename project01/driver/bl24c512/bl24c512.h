#ifndef __BL24C512_H__
#define __BL24C512_H__

#include "stm32f4xx.h"
#include "stdint.h"
#include "stdbool.h"

/*
BL24C512 EEPROM（I2C，8 位地址 0xA0，页大小 128 字节）
总线：默认走 i2c.c 里的 i2c1_bus（I2C1，PB6=SCL PB7=SDA）；换引脚改 i2c1_bus 即可。
用前须先调用 i2c_init(&i2c1_bus)。
*/

bool BL24C512_PageWrite(uint16_t address, uint8_t data[], uint32_t length);
bool BL24C512_write(uint16_t address, uint8_t data[], uint32_t length);
bool BL24C512_read(uint16_t address, uint8_t data[], uint32_t length);

#endif
