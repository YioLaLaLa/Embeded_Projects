#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include "main.h"

#define WAVE_GAIN_LEVEL_COUNT      6
#define SPECTRUM_GAIN_LEVEL_COUNT  6

float App_GetWaveGain(void);
float App_GetSpectrumGain(void);

uint8_t App_GetWaveGainIndex(void);
uint8_t App_GetSpectrumGainIndex(void);

const char *App_GetWaveGainName(void);
const char *App_GetSpectrumGainName(void);

void App_WaveGainUp(void);
void App_WaveGainDown(void);
void App_SpectrumGainUp(void);
void App_SpectrumGainDown(void);

#endif
