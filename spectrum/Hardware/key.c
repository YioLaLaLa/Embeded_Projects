#include "key.h"

#define KEY_UP_PORT     GPIOE
#define KEY_UP_PIN      GPIO_PIN_13

#define KEY_DOWN_PORT   GPIOE
#define KEY_DOWN_PIN    GPIO_PIN_14

#define KEY_OK_PORT     GPIOE
#define KEY_OK_PIN      GPIO_PIN_15

static KeyEvent_t g_keyEvent = KEY_EVENT_NONE;

//结构体类型 KeyState_t，用来描述一个按键的状态
typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    uint8_t stableState;
    uint8_t lastRawState;
    uint32_t lastChangeTick;
    KeyEvent_t event;
} KeyState_t;

/** 
 * @brief 定义 UP Down OK按键的状态变量
 * 结构体变量 g_keyUp，g_keyDown，g_keyOk
*/
static KeyState_t g_keyUp =
{
    KEY_UP_PORT, KEY_UP_PIN, 0, 0, 0, KEY_EVENT_UP
};

static KeyState_t g_keyDown =
{
    KEY_DOWN_PORT, KEY_DOWN_PIN, 0, 0, 0, KEY_EVENT_DOWN
};

static KeyState_t g_keyOk =
{
    KEY_OK_PORT, KEY_OK_PIN, 0, 0, 0, KEY_EVENT_OK
};

//函数声明
static void Key_Update(KeyState_t *key);

/**
 * @brief 读取按键状态，返回1表示按下，0表示未按下
 * @param GPIO_TypeDef *GPIOx: GPIO端口
 * @param uint16_t GPIO_Pin: GPIO引脚
 * @return uint8_t: 1表示按下，0表示未按下
 */
static uint8_t Key_IsPressed(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    return (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET);
}

/**
 * @brief 初始化按键状态
 * @param None
 * @return None
 */
void Key_Init(void)
{
    g_keyEvent = KEY_EVENT_NONE;
    g_keyUp.stableState = 0;
    g_keyUp.lastRawState = 0;
    g_keyUp.lastChangeTick = 0;

    g_keyDown.stableState = 0;
    g_keyDown.lastRawState = 0;
    g_keyDown.lastChangeTick = 0;

    g_keyOk.stableState = 0;
    g_keyOk.lastRawState = 0;
    g_keyOk.lastChangeTick = 0;
}

/**
 * @brief 扫描按键状态，更新按键事件
 * @param None
 * @return None
 */
void Key_Scan(void)
{
    Key_Update(&g_keyUp);
    Key_Update(&g_keyDown);
    Key_Update(&g_keyOk);
}

/**
 * @brief 获取按键事件
 * @param None
 * @return KeyEvent_t: 按键事件（是枚举类型）
 */
KeyEvent_t Key_GetEvent(void)
{
    KeyEvent_t event = g_keyEvent;
    g_keyEvent = KEY_EVENT_NONE;
    return event;
}

/**
 * @brief 更新按键状态，进行去抖动处理，并设置按键事件
 * @param KeyState_t *key: 按键状态结构体指针
 * @return None
 */
static void Key_Update(KeyState_t *key)//参数是结构体变量地址
{
    uint8_t rawState;
    uint32_t now;

    if (key == 0)
    {
        return;
    }

    //结构体指针访问成员key->port，key->pin
    rawState = Key_IsPressed(key->port, key->pin);
    now = HAL_GetTick();

    //判断是否按下
    if (rawState != key->lastRawState)
    {
        key->lastRawState = rawState;
        key->lastChangeTick = now;
    }

    if ((now - key->lastChangeTick) < 20U)
    {
        return;
    }

    if (key->stableState != rawState)
    {
        key->stableState = rawState;

        if ((rawState != 0U) && (g_keyEvent == KEY_EVENT_NONE))
        {
            g_keyEvent = key->event;
        }
    }
}
