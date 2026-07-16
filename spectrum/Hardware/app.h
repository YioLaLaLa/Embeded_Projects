#ifndef __APP_H
#define __APP_H

#include <stdint.h>

void App_Init(void);
void App_Loop(void);

void App_AdjustWaveGain(int8_t step);
void App_AdjustSpectrumGain(int8_t step);

#endif
