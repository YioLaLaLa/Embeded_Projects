#ifndef __BOARD_H__
#define __BOARD_H__

/* 板级初始化层：把各外设驱动的 init 收拢到一处，main 只面向 board 接口。
   分两级：lowlevel 保证"能延时、能打日志"，board 把板上外设全部拉起。 */

/* 最低层初始化：系统延时 + 调试串口(USART1) + 开机横幅。
   必须最先调用，之后才允许 USART1_SendString 打日志。 */
void board_lowlevel_init(void);

/* 板上外设初始化，顺序：LCD(ST7789+SPI2) → ESP32 串口(USART2) →
   I2C1 总线 → AHT20 传感器 → RTC。每步结果经调试串口打印。
   不含联网/对时等业务动作；调用前须先 board_lowlevel_init()。 */
void board_init(void);

#endif /* __BOARD_H__ */
