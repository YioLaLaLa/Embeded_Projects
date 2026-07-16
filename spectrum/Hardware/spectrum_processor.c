#include "spectrum_processor.h"
#include "arm_math.h"
#include <math.h>
#include "app_config.h"

#define ADC_REF_MV                     3300.0f
#define ADC_MAX_COUNT                  4095.0f

/*
 * Full-scale reference used by the current display logic:
 * PA0 at about 3000mVpp is treated as the 0dB reference.
 */
#define SPECTRUM_FS_P2P_MV_AT_PA0      3000.0f

/* Fixed dB display range */
#define SPECTRUM_DB_TOP                 0.0f
#define SPECTRUM_DB_BOTTOM            -60.0f

/* Top row is text; the bars use about 56 vertical pixels */
#define SPECTRUM_HEIGHT_PIXELS         56.0f

/*
 * Optional calibration factor for matching the displayed dB value to the
 * measured hardware amplitude. Keep 1.0f unless calibration is needed.
 */
#define SPECTRUM_CAL_FACTOR             1.0f

static float32_t fftInput[FFT_SIZE];
static float32_t fftOutput[FFT_SIZE];
static float32_t fftMag[(FFT_SIZE / 2) + 1];

static uint16_t sampleIndex = 0;
static uint8_t spectrumReady = 0;
static uint8_t spectrumDisplay[SPECTRUM_BAR_COUNT];
static uint16_t dominantBin = 0;
static int16_t peakDb10 = -600;

static arm_rfft_fast_instance_f32 g_fftHandler;
static float32_t g_refMag = 1.0f;

static void Spectrum_Compute(void);
static float32_t HannWindow(uint16_t n, uint16_t N);

void Spectrum_Init(void)
{
    uint16_t i;
    float32_t aPeakCount;

    sampleIndex = 0;
    spectrumReady = 0;
    dominantBin = 0;
    peakDb10 = -600;

    arm_rfft_fast_init_f32(&g_fftHandler, FFT_SIZE);

    for (i = 0; i < FFT_SIZE; i++)
    {
        fftInput[i] = 0.0f;
        fftOutput[i] = 0.0f;
    }

    for (i = 0; i < (FFT_SIZE / 2) + 1; i++)
    {
        fftMag[i] = 0.0f;
    }

    for (i = 0; i < SPECTRUM_BAR_COUNT; i++)
    {
        spectrumDisplay[i] = 0;
    }

    /*
     * 3000mVpp at PA0
     * -> 1500mV peak
     * -> convert to ADC peak counts
     */
    aPeakCount = (SPECTRUM_FS_P2P_MV_AT_PA0 * 0.5f) * ADC_MAX_COUNT / ADC_REF_MV;

    /*
     * With a Hann window, a bin-centered sine has a main-peak magnitude
     * approximately equal to Apeak * N / 4.
     */
    g_refMag = aPeakCount * ((float32_t)FFT_SIZE / 4.0f) * SPECTRUM_CAL_FACTOR;

    if (g_refMag < 1.0f)
    {
        g_refMag = 1.0f;
    }
}

void Spectrum_ProcessSamples(uint16_t *src, uint16_t len)
{
    uint16_t i;
    float32_t dc = 0.0f;

    if (src == 0 || len == 0)
    {
        return;
    }

    for (i = 0; i < len; i++)
    {
        dc += (float32_t)src[i];
    }
    dc /= (float32_t)len;

    for (i = 0; i < len; i++)
    {
        float32_t x = (float32_t)src[i] - dc;

        fftInput[sampleIndex] = x * HannWindow(sampleIndex, FFT_SIZE);
        sampleIndex++;

        if (sampleIndex >= FFT_SIZE)
        {
            sampleIndex = 0;
            Spectrum_Compute();
            spectrumReady = 1;
        }
    }
}

static void Spectrum_Compute(void)
{
    uint16_t i;
    uint16_t bar;
    float32_t tempMax = 0.0f;
    uint16_t tempDominant = 0;
    float32_t peakDb;
    float32_t displayPeakDb;
    float32_t spectrumGain;

    arm_rfft_fast_f32(&g_fftHandler, fftInput, fftOutput, 0);

    /*
     * arm_rfft_fast_f32 output layout:
     * fftOutput[0] = DC
     * fftOutput[1] = Nyquist
     * fftOutput[2*k], fftOutput[2*k+1] are real/imag for bin k
     */
    fftMag[0] = fabsf(fftOutput[0]);
    fftMag[FFT_SIZE / 2] = fabsf(fftOutput[1]);

    for (i = 1; i < FFT_SIZE / 2; i++)
    {
        float32_t real = fftOutput[2 * i];
        float32_t imag = fftOutput[2 * i + 1];
        fftMag[i] = sqrtf(real * real + imag * imag);
    }

    /* Ignore DC and search the dominant bin from bin 1 upward */
    for (i = 1; i < SPECTRUM_MAX_BIN; i++)
    {
        if (fftMag[i] > tempMax)
        {
            tempMax = fftMag[i];
            tempDominant = i;
        }
    }

    dominantBin = tempDominant;

    if (tempMax < 1.0f)
    {
        tempMax = 1.0f;
    }

    /*
     * 真实峰值dB：
     * g_refMag 对应 3000mVpp 输入时的FFT幅度。
     * 这里不乘频谱增益，保证 D:-xx.x 是真实幅度显示。
     */
    peakDb = 20.0f * log10f(tempMax / g_refMag);

    if (peakDb > 0.0f) peakDb = 0.0f;
    if (peakDb < -99.9f) peakDb = -99.9f;

    peakDb10 = (int16_t)(peakDb * 10.0f);

    /*
     * 显示峰值dB：
     * 只用于频谱柱子的显示高度和弱柱抑制。
     *
     * 3000mVpp x1  = 满柱
     * 1500mVpp x2  = 满柱
     * 750mVpp  x4  = 满柱
     * 375mVpp  x8  = 满柱
     */
    spectrumGain = App_GetSpectrumGain();

    displayPeakDb = peakDb + 20.0f * log10f(spectrumGain);

    if (displayPeakDb > 0.0f) displayPeakDb = 0.0f;
    if (displayPeakDb < -99.9f) displayPeakDb = -99.9f;

    for (bar = 0; bar < SPECTRUM_BAR_COUNT; bar++)
    {
        uint16_t start = (bar * SPECTRUM_MAX_BIN) / SPECTRUM_BAR_COUNT;
        uint16_t end   = ((bar + 1) * SPECTRUM_MAX_BIN) / SPECTRUM_BAR_COUNT;
        float32_t barPower = 0.0f;
        float32_t barRms;
        float32_t db;
        float32_t norm;
        uint8_t newHeight;
        uint16_t k;

        if (end <= start)
        {
            end = start + 1;
        }

        for (k = start; k < end; k++)
        {
            barPower += fftMag[k] * fftMag[k];
        }

        barRms = sqrtf(barPower);

        if (barRms < 1.0f)
        {
            barRms = 1.0f;
        }

        /*
         * 频谱柱显示用dB：
         * 真实FFT幅度不变，只在显示映射时乘 spectrumGain。
         */
        db = 20.0f * log10f((barRms * spectrumGain) / g_refMag);

        /*
         * 抑制远低于显示峰值的杂散柱，使画面稳定。
         */
        if (db < (displayPeakDb - 25.0f))
        {
            db = SPECTRUM_DB_BOTTOM;
        }

        if (db > SPECTRUM_DB_TOP) db = SPECTRUM_DB_TOP;
        if (db < SPECTRUM_DB_BOTTOM) db = SPECTRUM_DB_BOTTOM;

        norm = (db - SPECTRUM_DB_BOTTOM) / (SPECTRUM_DB_TOP - SPECTRUM_DB_BOTTOM);

        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;

        /*
         * 这里不要再乘 spectrumGain。
         * 增益已经在 db = 20log10((barRms * spectrumGain) / g_refMag) 里生效。
         */
        newHeight = (uint8_t)(norm * SPECTRUM_HEIGHT_PIXELS);

        if (newHeight > 56)
        {
            newHeight = 56;
        }

        /* Rise faster, decay slower */
        if (newHeight > spectrumDisplay[bar])
        {
            spectrumDisplay[bar] = (uint8_t)(0.60f * spectrumDisplay[bar] + 0.40f * newHeight);
        }
        else
        {
            spectrumDisplay[bar] = (uint8_t)(0.85f * spectrumDisplay[bar] + 0.15f * newHeight);
        }
    }
}


static float32_t HannWindow(uint16_t n, uint16_t N)
{
    return 0.5f - 0.5f * cosf(2.0f * 3.1415926f * n / (N - 1));
}

uint8_t Spectrum_IsReady(void)
{
    return spectrumReady;
}

void Spectrum_ClearReady(void)
{
    spectrumReady = 0;
}

uint8_t* Spectrum_GetDisplayBuffer(void)
{
    return spectrumDisplay;
}

uint16_t Spectrum_GetDominantBin(void)
{
    return dominantBin;
}

int16_t Spectrum_GetPeakDb10(void)
{
    return peakDb10;
}
