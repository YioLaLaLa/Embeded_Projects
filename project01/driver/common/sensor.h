#ifndef __SENSOR_H__
#define __SENSOR_H__

#include "stdbool.h"

/* 温湿度传感器接口（vtable）：board/app 面向接口编程，
   具体实现（AHT20，将来 SHT30/BME280 等）各提供一个 const sensor_t 实例。 */
typedef struct {
    const char *name;                              /* 型号名，日志用 */
    bool (*init)(void);                            /* 上电初始化/校准 */
    bool (*read)(float *humidity, float *temperature);  /* 触发一次测量：RH% 与 °C */
} sensor_t;

#endif /* __SENSOR_H__ */
