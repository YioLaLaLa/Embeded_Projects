#ifndef __BL24C512_H__
#define __BL24C512_H__

#include "stm32f4xx.h"
#include "stdint.h"
#include "stdbool.h"

bool BL24C512_PageWrite(uint16_t address, uint8_t data[], uint32_t length);
bool BL24C512_write(uint16_t address, uint8_t data[], uint32_t length);
bool BL24C512_read(uint16_t address, uint8_t data[], uint32_t length);

#endif
