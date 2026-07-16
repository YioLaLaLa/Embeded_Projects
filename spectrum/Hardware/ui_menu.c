#include "ui_menu.h"
#include "key.h"
#include "app.h"

static UI_Page_t g_currentPage = UI_PAGE_MENU;
static MenuItem_t g_currentMenuItem = MENU_ITEM_WAVEFORM;
static UI_SettingsItem_t g_currentSettingsItem = UI_SETTINGS_ITEM_WAVE;

void UI_Init(void)
{
    g_currentPage = UI_PAGE_MENU;
    g_currentMenuItem = MENU_ITEM_WAVEFORM;
    g_currentSettingsItem = UI_SETTINGS_ITEM_WAVE;
}

void UI_Process(void)
{
    KeyEvent_t key = Key_GetEvent();

    if (key == KEY_EVENT_NONE)
    {
        return;
    }

    if (g_currentPage == UI_PAGE_MENU)
    {
        if (key == KEY_EVENT_UP)
        {
            if (g_currentMenuItem == MENU_ITEM_WAVEFORM)
            {
                g_currentMenuItem = MENU_ITEM_INFO;
            }
            else
            {
                g_currentMenuItem = (MenuItem_t)(g_currentMenuItem - 1);
            }
        }
        else if (key == KEY_EVENT_DOWN)
        {
            if (g_currentMenuItem == MENU_ITEM_INFO)
            {
                g_currentMenuItem = MENU_ITEM_WAVEFORM;
            }
            else
            {
                g_currentMenuItem = (MenuItem_t)(g_currentMenuItem + 1);
            }
        }
        else if (key == KEY_EVENT_OK)
        {
            switch (g_currentMenuItem)
            {
                case MENU_ITEM_WAVEFORM:
                    g_currentPage = UI_PAGE_WAVEFORM;
                    break;

                case MENU_ITEM_SPECTRUM:
                    g_currentPage = UI_PAGE_SPECTRUM;
                    break;

                case MENU_ITEM_SETTINGS:
                    g_currentPage = UI_PAGE_SETTINGS;
                    g_currentSettingsItem = UI_SETTINGS_ITEM_WAVE;
                    break;

                case MENU_ITEM_INFO:
                    g_currentPage = UI_PAGE_INFO;
                    break;

                default:
                    break;
            }
        }
    }
    else if (g_currentPage == UI_PAGE_WAVEFORM)
    {
        if (key == KEY_EVENT_UP)
        {
            App_AdjustWaveGain(1);
        }
        else if (key == KEY_EVENT_DOWN)
        {
            App_AdjustWaveGain(-1);
        }
        else if (key == KEY_EVENT_OK)
        {
            g_currentPage = UI_PAGE_MENU;
        }
    }
    else if (g_currentPage == UI_PAGE_SPECTRUM)
    {
        if (key == KEY_EVENT_UP)
        {
            App_AdjustSpectrumGain(1);
        }
        else if (key == KEY_EVENT_DOWN)
        {
            App_AdjustSpectrumGain(-1);
        }
        else if (key == KEY_EVENT_OK)
        {
            g_currentPage = UI_PAGE_MENU;
        }
    }
    else if (g_currentPage == UI_PAGE_SETTINGS)
    {
        if (key == KEY_EVENT_UP)
        {
            if (g_currentSettingsItem == UI_SETTINGS_ITEM_WAVE)
            {
                App_AdjustWaveGain(1);
            }
            else if (g_currentSettingsItem == UI_SETTINGS_ITEM_SPECTRUM)
            {
                App_AdjustSpectrumGain(1);
            }
        }
        else if (key == KEY_EVENT_DOWN)
        {
            if (g_currentSettingsItem == UI_SETTINGS_ITEM_WAVE)
            {
                App_AdjustWaveGain(-1);
            }
            else if (g_currentSettingsItem == UI_SETTINGS_ITEM_SPECTRUM)
            {
                App_AdjustSpectrumGain(-1);
            }
        }
        else if (key == KEY_EVENT_OK)
        {
            if (g_currentSettingsItem == UI_SETTINGS_ITEM_WAVE)
            {
                g_currentSettingsItem = UI_SETTINGS_ITEM_SPECTRUM;
            }
            else if (g_currentSettingsItem == UI_SETTINGS_ITEM_SPECTRUM)
            {
                g_currentSettingsItem = UI_SETTINGS_ITEM_BACK;
            }
            else
            {
                g_currentPage = UI_PAGE_MENU;
                g_currentSettingsItem = UI_SETTINGS_ITEM_WAVE;
            }
        }
    }
    else if (g_currentPage == UI_PAGE_INFO)
    {
        if (key == KEY_EVENT_OK)
        {
            g_currentPage = UI_PAGE_MENU;
        }
    }
}

UI_Page_t UI_GetCurrentPage(void)
{
    return g_currentPage;
}

MenuItem_t UI_GetCurrentMenuItem(void)
{
    return g_currentMenuItem;
}

UI_SettingsItem_t UI_GetCurrentSettingsItem(void)
{
    return g_currentSettingsItem;
}
