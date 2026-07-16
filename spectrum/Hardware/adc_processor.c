#include "adc_processor.h"
#include "dsp_filters.h"
#include "spectrum_processor.h"
#include "app_config.h"

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim3;

static uint16_t adcBuffer[ADC_BUFFER_SIZE];

static volatile uint8_t adcHalfReady = 0;
static volatile uint8_t adcFullReady = 0;

static volatile uint16_t realTimeAdc = 0;
static volatile uint16_t currentDcValue = 2048;
static volatile uint16_t blockMinAdc = 4095;
static volatile uint16_t blockMaxAdc = 0;

static float dcEstimate = 2048.0f;

/* 1024点波形帧缓存 */
static uint16_t waveFrameBuffer[WAVE_FRAME_SIZE];
static uint16_t waveFrameIndex = 0;

static uint8_t waveYBuffer[128];

//static FilterState_t gFilter;

static void ADC_ProcessBlock(uint16_t *src, uint16_t len);
static void Waveform_AppendBlock(uint16_t *src, uint16_t len);
static void Waveform_Compress1024To128(void);

void ADC_ProcessorInit(void)
{
    uint16_t i;

    //Filter_Init(&gFilter);
    Spectrum_Init();

    for (i = 0; i < WAVE_DISPLAY_COLS; i++)
    {
        waveYBuffer[i] = WAVE_CENTER_Y;
    }


    for (i = 0; i < WAVE_FRAME_SIZE; i++)
    {
        waveFrameBuffer[i] = 2048;
    }

    adcHalfReady = 0;
    adcFullReady = 0;
    realTimeAdc = 0;
    currentDcValue = 2048;
    blockMinAdc = 4095;
    blockMaxAdc = 0;
    dcEstimate = 2048.0f;
    waveFrameIndex = 0;
}

void ADC_ProcessorStart(void)
{
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_Base_Start(&htim3) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adcBuffer, ADC_BUFFER_SIZE) != HAL_OK)
    {
        Error_Handler();
    }
}

void ADC_ProcessorTask(void)
{
    if (adcHalfReady)
    {
        adcHalfReady = 0;
        ADC_ProcessBlock(&adcBuffer[0], PROCESS_BLOCK_SIZE);
    }

    if (adcFullReady)
    {
        adcFullReady = 0;
        ADC_ProcessBlock(&adcBuffer[PROCESS_BLOCK_SIZE], PROCESS_BLOCK_SIZE);
    }
}

static void ADC_ProcessBlock(uint16_t *src, uint16_t len)
{
    uint32_t sum = 0;
    uint16_t i;
    uint16_t localMin = 4095;
    uint16_t localMax = 0;

    for (i = 0; i < len; i++)
    {
        uint16_t v = src[i];
        sum += v;

        if (v < localMin) localMin = v;
        if (v > localMax) localMax = v;
    }

    {
        float blockAvg = (float)sum / (float)len;
        dcEstimate = 0.98f * dcEstimate + 0.02f * blockAvg;
        currentDcValue = (uint16_t)dcEstimate;
    }

    blockMinAdc = localMin;
    blockMaxAdc = localMax;

    Waveform_AppendBlock(src, len);
    Spectrum_ProcessSamples(src, len);
}

static void Waveform_AppendBlock(uint16_t *src, uint16_t len)
{
    uint16_t i;

    for (i = 0; i < len; i++)
    {
        waveFrameBuffer[waveFrameIndex] = src[i];
        waveFrameIndex++;

        if (waveFrameIndex >= WAVE_FRAME_SIZE)
        {
            waveFrameIndex = 0;
            Waveform_Compress1024To128();
        }
    }
}

static void Waveform_Compress1024To128(void)
{
    uint16_t col;
    float center = dcEstimate;
    float gain = App_GetWaveGain();

    for (col = 0; col < WAVE_DISPLAY_COLS; col++)
    {
        uint16_t start = col * WAVE_COMPRESS_RATIO;
        uint16_t end = start + WAVE_COMPRESS_RATIO;
        uint16_t k;
        float sumMv = 0.0f;

        for (k = start; k < end; k++)
        {
            float adcDelta = (float)waveFrameBuffer[k] - center;
            float yMv = adcDelta * 3300.0f / 4095.0f;
            sumMv += yMv;
        }

        {
            float avgMv = sumMv / (float)WAVE_COMPRESS_RATIO;

            int screenY = (int)(WAVE_CENTER_Y
                - (avgMv / WAVE_FS_PEAK_MV_AT_PA0)
                * WAVE_PIXELS_HALF_RANGE
                * gain);

            if (screenY < WAVE_TOP_Y) screenY = WAVE_TOP_Y;
            if (screenY > WAVE_BOTTOM_Y) screenY = WAVE_BOTTOM_Y;

            waveYBuffer[col] = (uint8_t)screenY;
        }
    }
}


uint16_t ADC_GetRealtimeValue(void)
{
    return realTimeAdc;
}

uint16_t ADC_GetDcValue(void)
{
    return currentDcValue;
}

uint16_t ADC_GetMinValue(void)
{
    return blockMinAdc;
}

uint16_t ADC_GetMaxValue(void)
{
    return blockMaxAdc;
}

uint8_t* ADC_GetWaveBuffer(void)
{
    return waveYBuffer;
}

uint16_t ADC_GetWaveDisplayLength(void)
{
    return WAVE_DISPLAY_COLS;
}

void ADC_RefreshWaveDisplay(void)
{
    Waveform_Compress1024To128();
}

uint16_t ADC_GetWaveFrequency(void)
{
    static uint16_t lastFreq = 0;
    uint16_t bin = Spectrum_GetDominantBin();
    uint32_t freq = (uint32_t)bin * 625 / 10;   /* 62.5Hz/bin */

    if (lastFreq == 0)
    {
        lastFreq = (uint16_t)freq;
    }
    else
    {
        lastFreq = (uint16_t)((lastFreq * 3 + freq) / 4);
    }

    return lastFreq;
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
        adcHalfReady = 1;
        realTimeAdc = adcBuffer[PROCESS_BLOCK_SIZE / 2];
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
        adcFullReady = 1;
        realTimeAdc = adcBuffer[PROCESS_BLOCK_SIZE + PROCESS_BLOCK_SIZE / 2];
    }
}
