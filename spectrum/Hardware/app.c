#include "main.h"
#include "app.h"
#include "OLED.h"
#include "key.h"
#include "ui_menu.h"
#include "display_manager.h"
#include "adc_processor.h"
#include "app_config.h"

void App_Init(void)
{
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, "Starting...", OLED_6X8);
    OLED_Update();
    HAL_Delay(300);

    Key_Init();
    UI_Init();
    Display_Init();
    ADC_ProcessorInit();
    ADC_ProcessorStart();

    OLED_Clear();
    OLED_Update();
}

void App_Loop(void)
{
    Key_Scan();
    ADC_ProcessorTask();
    UI_Process();
    Display_Task();
}

void App_AdjustWaveGain(int8_t step)
{
    if (step > 0)
    {
        while (step > 0)
        {
            App_WaveGainUp();
            step--;
        }
    }
    else if (step < 0)
    {
        while (step < 0)
        {
            App_WaveGainDown();
            step++;
        }
    }

    ADC_RefreshWaveDisplay();
}

void App_AdjustSpectrumGain(int8_t step)
{
    if (step > 0)
    {
        while (step > 0)
        {
            App_SpectrumGainUp();
            step--;
        }
    }
    else if (step < 0)
    {
        while (step < 0)
        {
            App_SpectrumGainDown();
            step++;
        }
    }
}
