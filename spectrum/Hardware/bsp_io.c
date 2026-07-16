#include "bsp_io.h"

/*
 * LED:
 * PB6  -> LED1
 * PB7  -> LED2
 * PD8  -> LED3
 * 低电平有效
 *
 * KEY:
 * PE13 -> KEY1
 * PE14 -> KEY2
 * PE15 -> KEY3
 * 低电平有效
 */

/* ---------------- LED 定义 ---------------- */
#define LED1_PORT   GPIOB
#define LED1_PIN    GPIO_PIN_6

#define LED2_PORT   GPIOB
#define LED2_PIN    GPIO_PIN_7

#define LED3_PORT   GPIOD
#define LED3_PIN    GPIO_PIN_8

/* ---------------- KEY 定义 ---------------- */
#define KEY1_PORT   GPIOE
#define KEY1_PIN    GPIO_PIN_13

#define KEY2_PORT   GPIOE
#define KEY2_PIN    GPIO_PIN_14

#define KEY3_PORT   GPIOE
#define KEY3_PIN    GPIO_PIN_15

/* ---------------- LED 控制 ---------------- */
/* 低电平有效：RESET=亮，SET=灭 */

void LED1_On(void)
{
    HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);
}

void LED1_Off(void)
{
    HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);
}

void LED1_Toggle(void)
{
    HAL_GPIO_TogglePin(LED1_PORT, LED1_PIN);
}

void LED2_On(void)
{
    HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_RESET);
}

void LED2_Off(void)
{
    HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_SET);
}

void LED2_Toggle(void)
{
    HAL_GPIO_TogglePin(LED2_PORT, LED2_PIN);
}

void LED3_On(void)
{
    HAL_GPIO_WritePin(LED3_PORT, LED3_PIN, GPIO_PIN_RESET);
}

void LED3_Off(void)
{
    HAL_GPIO_WritePin(LED3_PORT, LED3_PIN, GPIO_PIN_SET);
}

void LED3_Toggle(void)
{
    HAL_GPIO_TogglePin(LED3_PORT, LED3_PIN);
}

void LED_AllOff(void)
{
    LED1_Off();
    LED2_Off();
    LED3_Off();
}

void LED_AllOn(void)
{
    LED1_On();
    LED2_On();
    LED3_On();
}

/* ---------------- KEY 读取 ---------------- */
/* 低电平有效：RESET=按下，SET=松开 */

uint8_t KEY1_IsPressed(void)
{
    return (HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN) == GPIO_PIN_RESET) ? 1U : 0U;
}

uint8_t KEY2_IsPressed(void)
{
    return (HAL_GPIO_ReadPin(KEY2_PORT, KEY2_PIN) == GPIO_PIN_RESET) ? 1U : 0U;
}

uint8_t KEY3_IsPressed(void)
{
    return (HAL_GPIO_ReadPin(KEY3_PORT, KEY3_PIN) == GPIO_PIN_RESET) ? 1U : 0U;
}

/* ---------------- 测试任务 ---------------- */
/* 按下哪个键，对应 LED 亮；松开则灭 */

void IO_TestTask(void)
{
    if (KEY1_IsPressed())
        LED1_On();
    else
        LED1_Off();

    if (KEY2_IsPressed())
        LED2_On();
    else
        LED2_Off();

    if (KEY3_IsPressed())
        LED3_On();
    else
        LED3_Off();
}
