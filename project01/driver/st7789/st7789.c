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

/* 控制引脚 GPIO 初始化：CS/RST/BLK 在 GPIOB，DC 在 GPIOC，推挽输出 */
static void st7789_gpio_init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);   /* DC 在 GPIOC */

    GPIO_InitTypeDef g;
    GPIO_StructInit(&g);
    g.GPIO_Mode  = GPIO_Mode_OUT;
    g.GPIO_OType = GPIO_OType_PP;
    g.GPIO_Speed = GPIO_High_Speed;
    g.GPIO_PuPd  = GPIO_PuPd_NOPULL;

    g.GPIO_Pin = ST7789_CS_PIN | ST7789_RST_PIN | ST7789_BLK_PIN;   /* GPIOB: CS/RST/BLK */
    GPIO_Init(GPIOB, &g);
    g.GPIO_Pin = ST7789_DC_PIN;                                     /* GPIOC: DC */
    GPIO_Init(GPIOC, &g);

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

/* 用单一颜色(RGB565)填充整屏：设窗后 CS 保持低、DC=1，连续发像素 */
void st7789_fill(uint16_t color)
{
    st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);

    uint8_t hi = (uint8_t)(color >> 8);     /* RGB565 高字节先发 */
    uint8_t lo = (uint8_t)(color & 0xFF);
    uint32_t total = (uint32_t)ST7789_WIDTH * ST7789_HEIGHT;

    CS_LOW();
    DC_DAT();                               /* 后续都是像素数据 */
    for(uint32_t i = 0; i < total; i++)
    {
        spi_rw(&lcd_spi, hi);                 /* 每像素 2 字节 */
        spi_rw(&lcd_spi, lo);
    }
    CS_HIGH();
}
