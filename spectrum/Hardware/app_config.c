#include "app_config.h"

static uint8_t g_waveGainIndex = 1;
static uint8_t g_spectrumGainIndex = 1;

/*
 * 波形显示增益：
 * 只影响OLED波形高度，不改变ADC原始数据。
 */
static const float waveGainTable[WAVE_GAIN_LEVEL_COUNT] =
{
    0.5f,
    1.0f,
    2.0f,
    4.0f,
    8.0f,
    16.0f
};

/*
 * 频谱显示增益：
 * 只影响OLED频谱柱高度，不改变ADC原始数据。
 *
 * 硬件最大输入 3000mVpp。
 * x1  时：3000mVpp 输入，柱子满量程。
 * x2  时：1500mVpp 输入，柱子满量程。
 * x4  时：750mVpp 输入，柱子满量程。
 * x8  时：375mVpp 输入，柱子满量程。
 * x16 时：187.5mVpp 输入，柱子满量程。
 */
static const float spectrumGainTable[SPECTRUM_GAIN_LEVEL_COUNT] =
{
    0.5f,
    1.0f,
    2.0f,
    4.0f,
    8.0f,
    16.0f
};

static const char *waveGainNameTable[WAVE_GAIN_LEVEL_COUNT] =
{
    "x0.5",
    "x1",
    "x2",
    "x4",
    "x8",
    "x16"
};

static const char *spectrumGainNameTable[SPECTRUM_GAIN_LEVEL_COUNT] =
{
    "x0.5",
    "x1",
    "x2",
    "x4",
    "x8",
    "x16"
};

float App_GetWaveGain(void)
{
    return waveGainTable[g_waveGainIndex];
}

float App_GetSpectrumGain(void)
{
    return spectrumGainTable[g_spectrumGainIndex];
}

uint8_t App_GetWaveGainIndex(void)
{
    return g_waveGainIndex;
}

uint8_t App_GetSpectrumGainIndex(void)
{
    return g_spectrumGainIndex;
}

const char *App_GetWaveGainName(void)
{
    return waveGainNameTable[g_waveGainIndex];
}

const char *App_GetSpectrumGainName(void)
{
    return spectrumGainNameTable[g_spectrumGainIndex];
}

void App_WaveGainUp(void)
{
    if (g_waveGainIndex < WAVE_GAIN_LEVEL_COUNT - 1)
    {
        g_waveGainIndex++;
    }
}

void App_WaveGainDown(void)
{
    if (g_waveGainIndex > 0)
    {
        g_waveGainIndex--;
    }
}

void App_SpectrumGainUp(void)
{
    if (g_spectrumGainIndex < SPECTRUM_GAIN_LEVEL_COUNT - 1)
    {
        g_spectrumGainIndex++;
    }
}

void App_SpectrumGainDown(void)
{
    if (g_spectrumGainIndex > 0)
    {
        g_spectrumGainIndex--;
    }
}
