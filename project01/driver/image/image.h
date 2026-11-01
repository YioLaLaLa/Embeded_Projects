#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "stdint.h"

/* ===== 图片资源：RGB565 字节流(每像素2字节，高字节在前) =====
   数据定义在各自的 .c 里，这里只做 extern 声明 + 尺寸宏。
   用 st7789_draw_rgb(x, y, W, H, 数组名) 绘制。 */

/* 猫图 240×240（数据在 cat_photo.c）*/
#define IMG_CAT_W  240
#define IMG_CAT_H  240
extern const unsigned char gImage_cat_photo[];

#endif /* __IMAGE_H__ */
