#ifndef __KEY_H
#define __KEY_H

#include "main.h"
#include <stdint.h>

typedef enum
{
    KEY_EVENT_NONE = 0,
    KEY_EVENT_UP,
    KEY_EVENT_DOWN,
    KEY_EVENT_OK
} KeyEvent_t;

void Key_Init(void);
void Key_Scan(void);
KeyEvent_t Key_GetEvent(void);

#endif
