#include "display_manager.h"
#include "OLED.h"
#include "ui_menu.h"
#include "adc_processor.h"
#include "spectrum_processor.h"
#include "app_config.h"

static void Display_DrawMenu(MenuItem_t item);
static void Display_DrawWaveformPage(void);
static void Display_DrawSpectrumPage(void);
static void Display_DrawSettingsPage(void);
static void Display_DrawInfoPage(void);

void Display_Init(void)
{
    OLED_Clear();
    OLED_Update();
}

void Display_Task(void)
{
    static uint32_t lastRefresh = 0;
    uint32_t now = HAL_GetTick();

    if (now - lastRefresh < 30)
    {
        return;
    }

    switch (UI_GetCurrentPage())
    {
        case UI_PAGE_MENU:
            Display_DrawMenu(UI_GetCurrentMenuItem());
            break;

        case UI_PAGE_WAVEFORM:
            Display_DrawWaveformPage();
            break;

        case UI_PAGE_SPECTRUM:
            Display_DrawSpectrumPage();
            break;

        case UI_PAGE_SETTINGS:
            Display_DrawSettingsPage();
            break;

        case UI_PAGE_INFO:
            Display_DrawInfoPage();
            break;

        default:
            break;
    }

    OLED_Update();
    lastRefresh = now;
}

// 菜单页面显示函数
static void Display_DrawMenu(MenuItem_t item)
{
    OLED_Clear();

    OLED_ShowString(0, 0, "Menu", OLED_6X8);

    OLED_ShowString(0, 16, (item == MENU_ITEM_WAVEFORM) ? ">" : " ", OLED_6X8);
    OLED_ShowString(8, 16, "Waveform", OLED_6X8);

    OLED_ShowString(0, 28, (item == MENU_ITEM_SPECTRUM) ? ">" : " ", OLED_6X8);
    OLED_ShowString(8, 28, "Spectrum", OLED_6X8);

    OLED_ShowString(0, 40, (item == MENU_ITEM_SETTINGS) ? ">" : " ", OLED_6X8);
    OLED_ShowString(8, 40, "Settings", OLED_6X8);

    OLED_ShowString(0, 52, (item == MENU_ITEM_INFO) ? ">" : " ", OLED_6X8);
    OLED_ShowString(8, 52, "Info", OLED_6X8);
}

// 波形页面显示函数
static void Display_DrawWaveformPage(void)
{
    uint16_t x;
    uint8_t *waveBuf = ADC_GetWaveBuffer();
    uint16_t waveLen = ADC_GetWaveDisplayLength();

    OLED_Clear();

    OLED_ShowString(0, 0, "A:", OLED_6X8);
    OLED_ShowNum(12, 0, ADC_GetRealtimeValue(), 4, OLED_6X8);

    OLED_ShowString(42, 0, "G:", OLED_6X8);
    OLED_ShowString(54, 0, App_GetWaveGainName(), OLED_6X8);

    OLED_ShowString(84, 0, "F:", OLED_6X8);
    OLED_ShowNum(96, 0, ADC_GetWaveFrequency(), 5, OLED_6X8);

    for (x = 1; x < waveLen; x++)
    {
        OLED_DrawLine(x - 1, waveBuf[x - 1], x, waveBuf[x]);
    }
}

// 频谱页面显示函数
static void Display_DrawSpectrumPage(void)
{
    uint8_t *spec = Spectrum_GetDisplayBuffer();
    uint16_t dom = Spectrum_GetDominantBin();
    uint32_t peakFreq = (uint32_t)dom * 625 / 10;   /* 62.5Hz/bin */
    int16_t db10 = Spectrum_GetPeakDb10();
    int16_t dbAbs;
    uint8_t dbInt;
    uint8_t dbFrac;
    uint8_t i;
    uint8_t y;

    OLED_Clear();

    if (db10 < 0)
    {
        dbAbs = -db10;
        OLED_ShowString(0, 0, "D:-", OLED_6X8);
    }
    else
    {
        dbAbs = db10;
        OLED_ShowString(0, 0, "D:+", OLED_6X8);
    }

    dbInt = dbAbs / 10;
    dbFrac = dbAbs % 10;

    OLED_ShowNum(18, 0, dbInt, 2, OLED_6X8);
    OLED_ShowChar(30, 0, '.', OLED_6X8);
    OLED_ShowNum(36, 0, dbFrac, 1, OLED_6X8);

    OLED_ShowString(54, 0, "F:", OLED_6X8);
    OLED_ShowNum(66, 0, peakFreq, 5, OLED_6X8);

    for (i = 0; i < SPECTRUM_BAR_COUNT; i++)
    {
        uint8_t barHeight = spec[i];

        if (barHeight > 56)
        {
            barHeight = 56;
        }

        for (y = 0; y < barHeight; y++)
        {
            OLED_DrawPoint(i, 63 - y);
        }
    }
}

static void Display_DrawSettingsPage(void)
{
    UI_SettingsItem_t item = UI_GetCurrentSettingsItem();

    OLED_Clear();

    OLED_ShowString(0, 0, "Settings", OLED_6X8);

    OLED_ShowString(0, 16, (item == UI_SETTINGS_ITEM_WAVE) ? ">" : " ", OLED_6X8);
    OLED_ShowString(8, 16, "Wave G:", OLED_6X8);
    OLED_ShowString(48, 16, App_GetWaveGainName(), OLED_6X8);

    OLED_ShowString(0, 28, (item == UI_SETTINGS_ITEM_SPECTRUM) ? ">" : " ", OLED_6X8);
    OLED_ShowString(8, 28, "Spec G:", OLED_6X8);
    OLED_ShowString(48, 28, App_GetSpectrumGainName(), OLED_6X8);

    OLED_ShowString(0, 44, (item == UI_SETTINGS_ITEM_BACK) ? ">" : " ", OLED_6X8);
    OLED_ShowString(8, 44, "Back", OLED_6X8);

    OLED_ShowString(0, 56, "U/D:Adj OK:Nxt", OLED_6X8);
}

static void Display_DrawInfoPage(void)
{
    OLED_Clear();

    OLED_ShowString(0, 0, "Info", OLED_6X8);

    OLED_ShowString(0, 16, "ADC:", OLED_6X8);
    OLED_ShowNum(30, 16, ADC_GetRealtimeValue(), 4, OLED_6X8);

    OLED_ShowString(0, 28, "DC :", OLED_6X8);
    OLED_ShowNum(30, 28, ADC_GetDcValue(), 4, OLED_6X8);

    OLED_ShowString(0, 40, "MIN:", OLED_6X8);
    OLED_ShowNum(30, 40, ADC_GetMinValue(), 4, OLED_6X8);

    OLED_ShowString(70, 40, "MAX:", OLED_6X8);
    OLED_ShowNum(100, 40, ADC_GetMaxValue(), 4, OLED_6X8);

    OLED_ShowString(0, 56, "OK:Back", OLED_6X8);
}
