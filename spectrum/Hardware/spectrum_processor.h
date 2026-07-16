#ifndef __SPECTRUM_PROCESSOR_H
#define __SPECTRUM_PROCESSOR_H

#include <stdint.h>

#define FFT_SIZE              1024
#define SPECTRUM_BAR_COUNT     128
#define SPECTRUM_MAX_BIN       320    /* 0~20kHz shown, Fs=64kHz, 62.5Hz/bin */

/* dB * 10, for example -253 means -25.3dB */
void Spectrum_Init(void);
void Spectrum_ProcessSamples(uint16_t *src, uint16_t len);

uint8_t Spectrum_IsReady(void);
void Spectrum_ClearReady(void);

uint8_t* Spectrum_GetDisplayBuffer(void);
uint16_t Spectrum_GetDominantBin(void);
int16_t Spectrum_GetPeakDb10(void);

#endif
