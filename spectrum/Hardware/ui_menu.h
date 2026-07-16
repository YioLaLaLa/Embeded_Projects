#ifndef __UI_MENU_H
#define __UI_MENU_H

#include <stdint.h>

typedef enum
{
    UI_PAGE_MENU = 0,
    UI_PAGE_WAVEFORM,
    UI_PAGE_SPECTRUM,
    UI_PAGE_SETTINGS,
    UI_PAGE_INFO
} UI_Page_t;

typedef enum
{
    MENU_ITEM_WAVEFORM = 0,
    MENU_ITEM_SPECTRUM,
    MENU_ITEM_SETTINGS,
    MENU_ITEM_INFO,
    MENU_ITEM_COUNT
} MenuItem_t;

typedef enum
{
    UI_SETTINGS_ITEM_WAVE = 0,
    UI_SETTINGS_ITEM_SPECTRUM,
    UI_SETTINGS_ITEM_BACK
} UI_SettingsItem_t;

void UI_Init(void);
void UI_Process(void);

UI_Page_t UI_GetCurrentPage(void);
MenuItem_t UI_GetCurrentMenuItem(void);
UI_SettingsItem_t UI_GetCurrentSettingsItem(void);

#endif
