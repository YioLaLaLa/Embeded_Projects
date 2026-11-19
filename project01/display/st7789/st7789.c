#include "st7789.h"
#include "SPI.h"
#include "delay.h"

/* 本屏使用的 SPI 实例：SPI2，PB13=SCK PB14=MISO PB15=MOSI，AF5，~10.5MHz */
static spi_t lcd_spi = {
    SPI2, SPI_BaudRatePrescaler_4,
    GPIOB,
    GPIO_Pin_13, GPIO_Pin_14, GPIO_Pin_15,
    GPIO_PinSource13, GPIO_PinSource14, GPIO_PinSource15,
    GPIO_AF_SPI2
};

/* ---- 像素突发通道：固定 8 位帧 + 字节 DMA(SPI2_TX) ----
   图片小端存储(lo,hi)，ST7789 要高字节先发：分块换序进 s_pxbuf 后整段 DMA。
   不切 16 位帧——F4 在 8 位模式下写半字 DR 会打包为“低字节先发两帧”
   (RM0090 §28.5.9)，DFF 切换一旦未按预期生效，每像素两字节颠倒→颜色全错。
   命令路径与像素路径同帧宽，无切换风险。 */
#define LCD_PX_CHUNK   480          /* 每批换算像素数(缓冲 960B)，须 ≥ 屏宽 */
#define LCD_DMA_EN     1            /* 0=逐字节轮询(与旧版行为一致)，A/B 对照用 */

static uint8_t s_pxbuf[LCD_PX_CHUNK * 2];

/* 已排好序的字节流出口：DMA 开走 spi_dma_send8，关则逐字节轮询 */
static void px_out(const uint8_t *buf, uint32_t nbytes)
{
#if LCD_DMA_EN
    spi_dma_send8(&lcd_spi, buf, nbytes);
#else
    for(uint32_t i = 0; i < nbytes; i++)
        spi_rw(&lcd_spi, buf[i]);
#endif
}

static void px_flush(void) { spi_flush_rx(&lcd_spi); }   /* 只发不收会 RX 溢出，突发后清一次 */

/* 纯色像素：分块填 hi,lo 图案进缓冲后发送 */
static void px_send_color(uint16_t color, uint32_t pixels)
{
    uint8_t hi = (uint8_t)(color >> 8), lo = (uint8_t)color;
    while(pixels)
    {
        uint32_t n = (pixels > LCD_PX_CHUNK) ? LCD_PX_CHUNK : pixels;
        for(uint32_t i = 0; i < n; i++) { s_pxbuf[i * 2] = hi; s_pxbuf[i * 2 + 1] = lo; }
        px_out(s_pxbuf, n * 2);
        pixels -= n;
    }
    px_flush();
}

/* 小端图片段：分块换序(hi 先发)进缓冲后发送；对源地址无对齐要求 */
static void px_send_le(const uint8_t *src, uint32_t pixels)
{
    while(pixels)
    {
        uint32_t n = (pixels > LCD_PX_CHUNK) ? LCD_PX_CHUNK : pixels;
        for(uint32_t i = 0; i < n; i++)
        {
            s_pxbuf[i * 2]     = src[i * 2 + 1];   /* 高字节先发 */
            s_pxbuf[i * 2 + 1] = src[i * 2];
        }
        px_out(s_pxbuf, n * 2);
        src    += n * 2;
        pixels -= n;
    }
    px_flush();
}

/* ==================== ST7789 命令定义（数据手册 9.1 / 9.2 节）==================== */
#define ST7789_SWRESET   0x01   /* Software Reset */
#define ST7789_SLPOUT    0x11   /* Sleep Out */
#define ST7789_NORON     0x13   /* Normal Display Mode On */
#define ST7789_INVON     0x21   /* Display Inversion On（ST7789 面板通常必须开，否则颜色反）*/
#define ST7789_DISPON    0x29   /* Display On */
#define ST7789_MADCTL    0x36   /* Memory Data Access Control（方向）*/
#define ST7789_COLMOD    0x3A   /* Interface Pixel Format（像素格式）*/
#define ST7789_PORCTRL   0xB2   /* Porch Setting */
#define ST7789_GCTRL     0xB7   /* Gate Control */
#define ST7789_VCOMS     0xBB   /* VCOMS Setting */
#define ST7789_LCMCTRL   0xC0   /* LCM Control */
#define ST7789_VDVVRHEN  0xC2   /* VDV and VRH Command Enable */
#define ST7789_VRHS      0xC3   /* VRH Set */
#define ST7789_VDVS      0xC4   /* VDV Set */
#define ST7789_FRCTRL2   0xC6   /* Frame Rate Control in Normal Mode */
#define ST7789_PWCTRL1   0xD0   /* Power Control 1 */
#define ST7789_PVGAMCTRL 0xE0   /* Positive Voltage Gamma Control */
#define ST7789_NVGAMCTRL 0xE1   /* Negative Voltage Gamma Control */

/* ==================== 控制引脚操作 ==================== */
#define CS_LOW()    GPIO_ResetBits(ST7789_CS_PORT,  ST7789_CS_PIN)
#define CS_HIGH()   GPIO_SetBits(ST7789_CS_PORT,    ST7789_CS_PIN)
#define DC_CMD()    GPIO_ResetBits(ST7789_DC_PORT,  ST7789_DC_PIN)   /* DC=0：命令 */
#define DC_DAT()    GPIO_SetBits(ST7789_DC_PORT,    ST7789_DC_PIN)   /* DC=1：数据 */
#define RST_LOW()   GPIO_ResetBits(ST7789_RST_PORT, ST7789_RST_PIN)
#define RST_HIGH()  GPIO_SetBits(ST7789_RST_PORT,   ST7789_RST_PIN)
#define BLK_ON()    GPIO_SetBits(ST7789_BLK_PORT,   ST7789_BLK_PIN)
#define BLK_OFF()   GPIO_ResetBits(ST7789_BLK_PORT, ST7789_BLK_PIN)

/* 写一条命令：CS 拉低 → DC=0 → 发字节 → CS 拉高 */
void st7789_write_cmd(uint8_t cmd)
{
    CS_LOW();
    DC_CMD();   //DC=0：命令
    spi_rw(&lcd_spi, cmd);
    CS_HIGH();
}

/* 写一个数据字节：CS 拉低 → DC=1 → 发字节 → CS 拉高 */
void st7789_write_data(uint8_t data)
{
    CS_LOW();
    DC_DAT();   //DC=1：数据
    spi_rw(&lcd_spi, data);
    CS_HIGH();
}

/* 控制引脚 GPIO 初始化：CS/BLK 在 GPIOB，DC 在 GPIOC，RST 在 GPIOD，推挽输出 */
static void st7789_gpio_init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);   /* DC 在 GPIOC */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);   /* RST 在 GPIOD */

    GPIO_InitTypeDef g;
    GPIO_StructInit(&g);
    g.GPIO_Mode  = GPIO_Mode_OUT;
    g.GPIO_OType = GPIO_OType_PP;
    g.GPIO_Speed = GPIO_High_Speed;
    g.GPIO_PuPd  = GPIO_PuPd_NOPULL;

    g.GPIO_Pin = ST7789_CS_PIN | ST7789_BLK_PIN;            /* GPIOB: CS/BLK */
    GPIO_Init(GPIOB, &g);
    g.GPIO_Pin = ST7789_DC_PIN;                             /* GPIOC: DC */
    GPIO_Init(GPIOC, &g);
    g.GPIO_Pin = ST7789_RST_PIN;                            /* GPIOD: RST */
    GPIO_Init(GPIOD, &g);

    CS_HIGH();      /* CS 默认未选中 */
    RST_HIGH();
    BLK_OFF();      /* 先关背光，等 init 末尾配置完再点亮，避免上电白闪 */
}

/* 硬件复位时序 */
static void st7789_hw_reset(void)
{
    RST_HIGH();  Delay_ms(10);
    RST_LOW();   Delay_ms(10);      /* 复位低脉冲 */
    RST_HIGH();  Delay_ms(120);     /* 手册：RESX 释放后等 ≥5ms 才能发命令，这里给足 120ms */
}

void st7789_init(void)
{
    spi_init(&lcd_spi);       /* 先初始化 SPI2（PB13/14/15）*/
    st7789_gpio_init();     /* 配置 CS/DC/RST/BLK */
    st7789_hw_reset();      /* 硬件复位 */

    st7789_write_cmd(ST7789_SWRESET);                 /* 软件复位 */
    Delay_ms(150);                                    /* 手册：软复位后等 5ms 载默认值；sleep-in 下需 120ms 才能 SLPOUT，这里给 150ms */

    st7789_write_cmd(ST7789_SLPOUT);                  /* 退出睡眠 */
    Delay_ms(120);

    st7789_write_cmd(ST7789_COLMOD);                  /* 像素格式 */
    st7789_write_data(0x55);                          /* 手册 Note2：写 16-bit/像素时 3Ah=55h（RGB565）*/

    st7789_write_cmd(ST7789_MADCTL);                  /* 内存访问方向 */
    st7789_write_data(ST7789_ORIENTATION);            /* 默认 0x00 竖屏正显 */

    st7789_write_cmd(ST7789_PORCTRL);                 /* Porch 设置 */
    st7789_write_data(0x0C); st7789_write_data(0x0C);
    st7789_write_data(0x00); st7789_write_data(0x33); st7789_write_data(0x33);

    st7789_write_cmd(ST7789_GCTRL);                   /* 门控电压 */
    st7789_write_data(0x35);

    st7789_write_cmd(ST7789_VCOMS);                   /* VCOMS 设置 */
    st7789_write_data(0x19);

    st7789_write_cmd(ST7789_LCMCTRL);                 /* LCM 控制 */
    st7789_write_data(0x2C);

    st7789_write_cmd(ST7789_VDVVRHEN);                /* VDV/VRH 命令使能 */
    st7789_write_data(0x01);

    st7789_write_cmd(ST7789_VRHS);                    /* VRH 设置 */
    st7789_write_data(0x12);

    st7789_write_cmd(ST7789_VDVS);                    /* VDV 设置 */
    st7789_write_data(0x20);

    st7789_write_cmd(ST7789_FRCTRL2);                 /* 正常模式帧率 */
    st7789_write_data(0x0F);

    st7789_write_cmd(ST7789_PWCTRL1);                 /* 电源控制 1 */
    st7789_write_data(0xA4); st7789_write_data(0xA1);

    st7789_write_cmd(ST7789_PVGAMCTRL);               /* 正极性 Gamma（14 字节）*/
    st7789_write_data(0xD0); st7789_write_data(0x04); st7789_write_data(0x0D); st7789_write_data(0x11);
    st7789_write_data(0x13); st7789_write_data(0x2B); st7789_write_data(0x3F); st7789_write_data(0x54);
    st7789_write_data(0x4C); st7789_write_data(0x18); st7789_write_data(0x0D); st7789_write_data(0x0B);
    st7789_write_data(0x1F); st7789_write_data(0x23);

    st7789_write_cmd(ST7789_NVGAMCTRL);               /* 负极性 Gamma（14 字节）*/
    st7789_write_data(0xD0); st7789_write_data(0x04); st7789_write_data(0x0C); st7789_write_data(0x11);
    st7789_write_data(0x13); st7789_write_data(0x2C); st7789_write_data(0x3F); st7789_write_data(0x44);
    st7789_write_data(0x51); st7789_write_data(0x2F); st7789_write_data(0x1F); st7789_write_data(0x1F);
    st7789_write_data(0x20); st7789_write_data(0x23);

    st7789_write_cmd(ST7789_INVON);                   /* 反显开（ST7789 面板需要）*/
    Delay_ms(10);

    st7789_write_cmd(ST7789_NORON);                   /* 正常显示模式开 */
    Delay_ms(10);

    st7789_write_cmd(ST7789_DISPON);                  /* 显示开 */
    Delay_ms(120);

    BLK_ON();                                         /* 确保背光亮 */
}

/* 设置显示窗口：CASET(2Ah) 列范围、RASET(2Bh) 行范围，再 RAMWR(2Ch) 准备写显存 */
static void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    st7789_write_cmd(0x2A);                 /* CASET：列地址范围 */
    st7789_write_data(x0 >> 8);
    st7789_write_data(x0 & 0xFF);
    st7789_write_data(x1 >> 8);
    st7789_write_data(x1 & 0xFF);

    st7789_write_cmd(0x2B);                 /* RASET：行地址范围 */
    st7789_write_data(y0 >> 8);
    st7789_write_data(y0 & 0xFF);
    st7789_write_data(y1 >> 8);
    st7789_write_data(y1 & 0xFF);

    st7789_write_cmd(0x2C);                 /* RAMWR：开始写显存 */
}

/* 用单一颜色(RGB565)填充整屏：设窗后分块发 hi,lo 图案 */
void st7789_fill(uint16_t color)
{
    st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);

    CS_LOW();
    DC_DAT();                               /* 后续都是像素数据 */
    px_send_color(color, (uint32_t)ST7789_WIDTH * ST7789_HEIGHT);
    CS_HIGH();
}

/* 在指定矩形区域 (x0,y0)-(x1,y1) 填充单一颜色（复用 set_window 框局部窗口）*/
void st7789_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    st7789_set_window(x0, y0, x1, y1);

    uint32_t total = (uint32_t)(x1 - x0 + 1) * (y1 - y0 + 1);   /* 窗口内像素数 */

    CS_LOW();
    DC_DAT();
    px_send_color(color, total);
    CS_HIGH();
}

/* 在 (x,y) 画 w×h 单色位图：bit=1 用 fg，bit=0 用 bg（行列式/MSB先）。
   逐行展开进 s_pxbuf 后整行 DMA。w 须 ≤ LCD_PX_CHUNK(480)，字体最大 32 满足。 */
void st7789_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        const uint8_t *bmp, uint16_t fg, uint16_t bg)
{
    st7789_set_window(x, y, x + w - 1, y + h - 1);

    uint16_t row_bytes = (w + 7) / 8;              /* 每行字节数 */
    uint8_t fghi = (uint8_t)(fg >> 8), fglo = (uint8_t)fg;
    uint8_t bghi = (uint8_t)(bg >> 8), bglo = (uint8_t)bg;

    CS_LOW();
    DC_DAT();
    for(uint16_t r = 0; r < h; r++)
    {
        for(uint16_t c = 0; c < w; c++)
        {
            uint8_t byte = bmp[r * row_bytes + c / 8];
            uint8_t bit  = (byte >> (7 - c % 8)) & 0x01;   /* MSB first */
            s_pxbuf[c * 2]     = bit ? fghi : bghi;
            s_pxbuf[c * 2 + 1] = bit ? fglo : bglo;
        }
        px_out(s_pxbuf, (uint32_t)w * 2);
    }
    px_flush();
    CS_HIGH();
}

/* 在 (x,y) 画 w×h 全彩位图：img 是 RGB565 字节流(每像素2字节，小端)。
   分块换序后 DMA 发送，高字节先发。若颜色红蓝互反，是面板 RGB/BGR 问题，
   改 MADCTL 的 BGR 位。 */
void st7789_draw_rgb(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *img)
{
    st7789_set_window(x, y, x + w - 1, y + h - 1);

    CS_LOW();
    DC_DAT();
    px_send_le(img, (uint32_t)w * h);
    CS_HIGH();
}

/* 色键透明画全彩位图：逐行扫描，等于 key 色的像素跳过(露出背景)，
   连续非 key 像素合成一段设一次窗口流式写，减少 SPI 命令开销。
   图标 RGB565 数据自带黑底，用 key=ST7789_BLACK 即可只保留图形。*/
void st7789_draw_rgb_transparent(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                                 const uint8_t *img, uint16_t key)
{
    uint8_t khi = (uint8_t)(key >> 8);
    uint8_t klo = (uint8_t)(key & 0xFF);

    for(uint16_t r = 0; r < h; r++)
    {
        uint16_t c = 0;
        while(c < w)
        {
            uint32_t idx = (uint32_t)(r * w + c) * 2;
            /* 跳过 key 色像素 */
            if(img[idx] == klo && img[idx + 1] == khi) { c++; continue; }
            /* 找连续非 key 段的末尾 */
            uint16_t start = c;
            while(c < w)
            {
                uint32_t k = (uint32_t)(r * w + c) * 2;
                if(img[k] == klo && img[k + 1] == khi) break;
                c++;
            }
            st7789_set_window(x + start, y + r, x + c - 1, y + r);
            CS_LOW();
            DC_DAT();
            /* 段起点字节偏移恒为偶数；字节 DMA 对地址无对齐要求 */
            px_send_le(&img[(uint32_t)(r * w + start) * 2], c - start);
            CS_HIGH();
        }
    }
}

/* 透明画单色位图：只画 bit=1(笔画 fg)，bit=0(背景)跳过不写，露出底下背景。
   用于文字叠加在背景图上而不产生黑色底框。*/
void st7789_draw_bitmap_transparent(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                                    const uint8_t *bmp, uint16_t fg)
{
    uint16_t row_bytes = (w + 7) / 8;

    for(uint16_t r = 0; r < h; r++)
    {
        uint16_t c = 0;
        while(c < w)
        {
            if(!((bmp[r * row_bytes + c / 8] >> (7 - c % 8)) & 0x01)) { c++; continue; }
            uint16_t start = c;
            while(c < w && ((bmp[r * row_bytes + c / 8] >> (7 - c % 8)) & 0x01)) c++;
            st7789_set_window(x + start, y + r, x + c - 1, y + r);
            CS_LOW();
            DC_DAT();
            px_send_color(fg, c - start);   /* 纯色段 */
            CS_HIGH();
        }
    }
}

/* 从大图 img(宽 img_w，屏幕坐标与图内坐标一致，即图画在(0,0)) 取 (x,y) 起
   w×h 子区域重绘到屏幕同位置，用于透明文字/图标刷新前擦掉上一帧残影。*/
void st7789_restore_bg(const uint8_t *img, uint16_t img_w,
                       uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    st7789_set_window(x, y, x + w - 1, y + h - 1);
    CS_LOW();
    DC_DAT();
    for(uint16_t r = 0; r < h; r++)      /* 每行段在图内存中连续，逐行发送 */
        px_send_le(&img[((uint32_t)(y + r) * img_w + x) * 2], w);
    CS_HIGH();
}
