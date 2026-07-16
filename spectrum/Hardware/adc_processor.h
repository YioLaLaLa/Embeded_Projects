#ifndef __ADC_PROCESSOR_H
#define __ADC_PROCESSOR_H

#include "main.h"
#include <stdint.h>

#define ADC_BUFFER_SIZE      128
#define PROCESS_BLOCK_SIZE   (ADC_BUFFER_SIZE / 2)

#define OLED_W               128
#define WAVE_CENTER_Y        40
#define WAVE_TOP_Y           16
#define WAVE_BOTTOM_Y        63

#define WAVE_FRAME_SIZE      1024
#define WAVE_COMPRESS_RATIO  8
#define WAVE_DISPLAY_COLS    128

#define WAVE_FS_P2P_MV_AT_PA0    3000.0f
#define WAVE_FS_PEAK_MV_AT_PA0   (WAVE_FS_P2P_MV_AT_PA0 / 2.0f)
#define WAVE_PIXELS_HALF_RANGE   ((WAVE_BOTTOM_Y - WAVE_TOP_Y) / 2.0f)

void ADC_ProcessorInit(void);
void ADC_ProcessorStart(void);
void ADC_ProcessorTask(void);

uint16_t ADC_GetRealtimeValue(void);
uint16_t ADC_GetDcValue(void);
uint16_t ADC_GetMinValue(void);
uint16_t ADC_GetMaxValue(void);

uint8_t* ADC_GetWaveBuffer(void);
uint16_t ADC_GetWaveDisplayLength(void);
void ADC_RefreshWaveDisplay(void);

uint16_t ADC_GetWaveFrequency(void);

#endif
