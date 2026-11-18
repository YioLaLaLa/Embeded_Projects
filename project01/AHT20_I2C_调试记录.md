# AHT20 温湿度传感器 · I2C 调试问题记录

> **平台**：STM32F407VETx + 标准外设库（StdPeriph）+ Keil MDK（ARMCC）
> **硬件连接**：AHT20（I2C 7 位地址 `0x38`）挂在 I2C1，`PB6 = SCL`、`PB7 = SDA`；USART1（115200 8-N-1）用于打印调试信息
> **最终结果**：AHT20 读写全部修复，可稳定输出温湿度（实测约 **24 ℃ / 77 %RH**）
> **一句话结论**：所有故障都是**软件/驱动逻辑问题**，器件与接线自始至终都是好的；中途一度误判为"内部弱上拉"，走了弯路。

---

## 目录

- [一、涉及文件](#一涉及文件)
- [二、问题与解决（按定位顺序）](#二问题与解决按定位顺序)
  - [问题 1：总线扫描器"忙就跳过"，漏扫地址造成假阴性](#问题-1总线扫描器忙就跳过漏扫地址造成假阴性)
  - [问题 2：复位/重新烧录后 I2C 总线锁死（BUSY 永不消）](#问题-2复位重新烧录后-i2c-总线锁死busy-永不消)
  - [问题 3：单字节读 (N=1) 的 STOP 时序错误，收不到数据](#问题-3单字节读-n1-的-stop-时序错误收不到数据)
  - [问题 4：主机模式 STOPF 不可靠 + 失败路径未恢复 ACK](#问题-4主机模式-stopf-不可靠--失败路径未恢复-ack)
- [三、走过的弯路：一度误判为"内部弱上拉不足"](#三走过的弯路一度误判为内部弱上拉不足)
- [四、其他小问题：串口终端中文乱码](#四其他小问题串口终端中文乱码)
- [五、最终验证](#五最终验证)
- [六、当前配置与后续建议](#六当前配置与后续建议)
- [七、通用经验总结](#七通用经验总结)

---

## 一、涉及文件

| 文件 | 作用 |
|------|------|
| `driver/I2C/i2c.c` / `i2c.h` | I2C1 底层：初始化、总线恢复、`i2c_write` / `i2c_read` / `i2c_wait_flag` |
| `driver/AHT20/aht20.c` / `aht20.h` | AHT20 驱动：`AHT20_Init` / `AHT20_Read` / `AHT20_ReadStatus`（**本次未改动，逻辑本就正确**） |
| `driver/delay/*` | SysTick 延时 `Delay_Init` / `Delay_ms` / `Delay_us` |
| `driver/usart/*` | USART1 打印 `USART1_SendByte` / `USART1_SendString` |
| `app/main.c` | 调试期反复替换为诊断程序，最终为温湿度监视器 |

---

## 二、问题与解决（按定位顺序）

### 问题 1：总线扫描器"忙就跳过"，漏扫地址造成假阴性

**现象**
- 串口一直打印 `AHT20 read FAIL`；
- 自写的 I2C 总线扫描器（遍历 `0x08~0x77`）显示 `NO device ACK`，看起来整条总线一个器件都没有。

**根因**
扫描器的探测函数开头写成了"**忙就跳过这个地址**"：

```c
// ❌ 错误写法
if(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET) return false;  // 忙 → 直接跳过该地址
```

每探测完一个地址都会发一个 STOP，而 STOP 需要时间才完成；紧接着探测下一个地址时总线仍处于 `BUSY`，于是**该地址被直接跳过**。诊断时把时钟降到 20 kHz，STOP 完成更慢，漏扫极其严重，可能一整轮一个器件都扫不到——从而把排查方向**误导到"上拉/接线/供电"等硬件问题**上。

**解决**
探测前改为"**带超时地等待总线空闲**"，而不是忙就跳过；同时给 `SB` / `ADDR` 的等待设足够大的超时：

```c
// ✅ 正确写法
uint32_t timeout = 200000;
while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET)
{
    if(--timeout == 0) return false;   // 真的卡住才放弃
}
```

修好后立刻扫到 `ACK @0x38`，**证明器件一直在线、一直正常**。

**教训**
> 任何基于标志位轮询的 I2C 主机事务，两次事务之间必须确保上一次 STOP 已完成、总线回到空闲后再发起下一次。定位"无 ACK"时，**应先排除扫描/探测代码自身的时序缺陷，再怀疑硬件**。

---

### 问题 2：复位/重新烧录后 I2C 总线锁死（BUSY 永不消）

**现象**
- 分步诊断显示 `R-A BUSY FAIL (bus stuck)`、`W-A BUSY FAIL`；
- 即 `i2c_wait_flag(BUSY, RESET)` 等不到总线空闲，任何事务的第一步就失败。

**根因**
调试过程中多次重新烧录/复位。MCU 复位的瞬间，AHT20 可能正卡在一次读事务的中间、**把 SDA 拉低**输出某个数据位；复位后主机不再提供时钟，从机便一直拽住 SDA 不放 → 总线永久 `BUSY`。这是 I2C 的经典"总线锁死（bus lockup）"。

**解决**
在 `I2C1_Init()` 最前面加一次**总线恢复**：把 `PB6/PB7` 临时配成开漏 GPIO，若读到 SDA 为低，就手动打最多 **9 个 SCL 脉冲**逼从机把残留位时钟完、释放 SDA，最后补一个 STOP，再切回 I2C 复用功能。

```c
void I2C1_BusRecovery(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    uint8_t i;

    /* 临时把 PB6(SCL)/PB7(SDA) 配成开漏 GPIO 输出（写 1 即释放为高） */
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_SetBits(GPIOB, GPIO_Pin_6 | GPIO_Pin_7);   // 先释放两条线
    i2c_recovery_delay();

    for(i = 0; i < 9; i++)                          // SDA 若被拉低，最多打 9 个 SCL 脉冲
    {
        if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7) == Bit_SET) break;  // SDA 已恢复高
        GPIO_ResetBits(GPIOB, GPIO_Pin_6);          // SCL 低
        i2c_recovery_delay();
        GPIO_SetBits(GPIOB, GPIO_Pin_6);            // SCL 高
        i2c_recovery_delay();
    }

    GPIO_ResetBits(GPIOB, GPIO_Pin_7);              // 补 STOP：SDA 低
    i2c_recovery_delay();
    GPIO_SetBits(GPIOB, GPIO_Pin_6);                // SCL 高
    i2c_recovery_delay();
    GPIO_SetBits(GPIOB, GPIO_Pin_7);                // SDA 高 = STOP
    i2c_recovery_delay();
}
```

> `i2c_recovery_delay()` 是一个 `volatile` 计数空循环（约几 µs），让 `i2c.c` 不依赖 `delay` 模块。恢复完成后 `I2C1_Init()` 里打印的空闲电平为 `SCL=1 SDA=1`，`R-A bus free OK`，总线解锁成功。

**教训**
> 凡是会被反复烧录/复位、且从机可能在事务中途被打断的 I2C 主机，**上电初始化时就应做一次总线恢复**（9 脉冲 + STOP），这是健壮 I2C 驱动的标配。

---

### 问题 3：单字节读 (N=1) 的 STOP 时序错误，收不到数据

**现象**
- 诊断显示 `R-C ADDR OK (ACK read)`（读地址有应答）但 `R-D RXNE FAIL (no data)`（收不到那个字节）。

**根因**
`AHT20_ReadStatus()` 走的是"单字节读"（`i2c_read(..., 1)`）。原代码在**清 ADDR 之前**就发了 STOP：

```c
// ❌ 错误顺序（N=1）
I2C_AcknowledgeConfig(I2Cx, DISABLE);   // 关 ACK
I2C_GenerateSTOP(I2Cx, ENABLE);         // 先发 STOP
(void)I2Cx->SR2;                        // 再清 ADDR
```

ADDR 置位时 SCL 被拉住、字节还没开始收；此时排入的 STOP 会在清 ADDR 后**立即触发**，把这一个字节的接收直接掐断 → `RXNE` 永远等不到数据。
（多字节路径没有这个问题，因为它在 ADDR 早已清除、数据正在流动时才发 STOP。）

**解决**
按 RM0090 对 N=1 的要求，改为**先清 ADDR、再发 STOP**，且清 ADDR 要"读 SR1 紧接读 SR2"背靠背完成：

```c
if(len == 1)                                          // 单字节 N=1（RM0090 时序）
{
    I2C_AcknowledgeConfig(I2Cx, DISABLE);             // 1) 先关 ACK，NACK 这唯一字节
    (void)I2Cx->SR1;                                  // 2) 读 SR1
    (void)I2Cx->SR2;                                  //    紧接读 SR2 → 可靠清 ADDR
    I2C_GenerateSTOP(I2Cx, ENABLE);                   // 3) 清 ADDR 之后再 STOP（关键）
}
else
{
    (void)I2Cx->SR2;                                  // 多字节：清 ADDR，开始接收
}
```

修好后 `R-D RXNE OK status=0x18`，单字节读通了（`0x18`：bit3=1 已校准、bit7=0 空闲，是一颗健康的 AHT20）。

**教训**
> STM32 硬件 I2C 的**单字节读**是最容易踩坑的路径：必须 `关 ACK → 读 SR1+SR2 清 ADDR → 再 STOP → 等 RXNE 读 DR`，顺序错一步就收不到数据。

---

### 问题 4：主机模式 STOPF 不可靠 + 失败路径未恢复 ACK

**现象**
- 单字节读的数据到手后，`R-E STOPF FAIL`（等不到停止标志）；
- 紧接着的写操作 `W-C ADDR FAIL -> NACK on WRITE addr`（写地址被"拒绝"）。

**根因**（两件事连锁）
1. **STOPF 在主机模式下本就不可靠**（实测常常等不到）。STOP 其实已经发出、会在总线上自行完成，强行等 `STOPF` 只会超时失败。
2. 因为读在 `R-E` 提前 `return`，**没走到结尾的"重新使能 ACK"**，导致 ACK 被留在关闭状态。ACK 关着时，主机在地址阶段认不出从机的应答 → 下一次"写"的地址被误判成 NACK。（对比：扫描器能扫到 `0x70`，正是因为那时 ACK 是开着的。）

**解决**
- `i2c_read` 结尾**不再等 STOPF**，改由下一次事务开头的 `BUSY` 等待来确认 STOP 已完成、总线已释放：

```c
/* 主机模式下 STOPF 不可靠（实测等不到）：STOP 已发出会自行完成，
   由下一次传输开头的 BUSY 等待确认总线释放，此处不再等 STOPF */
I2C_AcknowledgeConfig(I2Cx, ENABLE);   // 恢复 ACK，供下次使用
return true;
```

- `i2c_write` **开头强制使能 ACK**，杜绝被上一次读关掉后遗留的问题：

```c
bool i2c_write(I2C_TypeDef *I2Cx, uint8_t dev_addr, const uint8_t *data, uint32_t len)
{
    if(!i2c_wait_flag(I2Cx, I2C_FLAG_BUSY, RESET)) return false;   // 等总线空闲
    I2C_AcknowledgeConfig(I2Cx, ENABLE);                          // 确保 ACK 使能：防止上次读把它关掉导致地址被误判 NACK
    I2C_GenerateSTART(I2Cx, ENABLE);
    ...
}
```

**教训**
> 主机模式**不要依赖 STOPF**判断结束；用"下一次事务等 BUSY 释放"来兜底更稳。同时要保证 **ACK 状态在事务之间一致**（每次事务开头显式设成需要的值），否则失败路径遗留的 ACK 关闭会污染下一次操作。

---

## 三、走过的弯路：一度误判为"内部弱上拉不足"

中途因为扫描器 bug（问题 1）显示 `NO device ACK`，加上空闲电平读到 `SCL=1 SDA=1`，一度怀疑是 **STM32 内部上拉约 40 kΩ 太弱、100 kHz 下上升沿过缓**导致从机采样不到，于是把时钟降到 20 kHz 验证。

**这个假设最终被证伪**：修好扫描器/总线恢复/读时序后，器件在**同样的内部上拉、20 kHz** 下就能正常 ACK、读到状态字节 `0x18`，说明**上拉是够用的、器件是健康的**，之前的"无 ACK"纯粹是软件 bug。

**保留的正确认识**：外部上拉（SDA/SCL 各 4.7 kΩ 到 3.3 V）对裸传感器仍是**推荐的硬件设计规范**，只是它**不是本次故障的原因**。

> **排查顺序纠正**：遇到"空闲电平高但无 ACK / 读不到数据"，应 **①先查扫描/探测代码缺陷 → ②查总线是否锁死 → ③查单字节读时序 → 最后才怀疑上拉电阻**。

---

## 四、其他小问题：串口终端中文乱码

**现象**：诊断信息里的中文在串口终端显示成 `???`（终端非 UTF-8）。
**解决**：所有诊断输出改用**英文 ASCII**（如 `R-A bus free OK`、`NO device ACK`），避免乱码干扰判读。正式监视器的固定文案也保持 ASCII。

---

## 五、最终验证

用"直接调用真实 `i2c_read` / `i2c_write`"的诊断程序验证 AHT20 用到的全部三种操作，全部通过：

```
===== Real i2c_read/write Diag (AHT20 @0x38) =====
[1] i2c_read(1) OK  status=0x18
[2] i2c_write(3) OK
[3] i2c_read(6) OK  raw=18 C5 F7 45 EE 6E
```

原始数据解码（`raw = 18 C5 F7 45 EE 6E`）：
- `buf[0]=0x18`：状态，bit7=0 不忙 ✓
- 湿度 = `0xC5F74 × 100 / 2²⁰` ≈ **77.3 %RH**
- 温度 = `0x5EE6E × 200 / 2²⁰ − 50` ≈ **24.1 ℃**

数值合理，驱动完全正常。

---

## 六、当前配置与后续建议

- **I2C 速度**：已从诊断用的 20 kHz **改回标准 100 kHz**（`i2c.c` 中 `I2C_ClockSpeed = 100000`）。
- **`app/main.c`**：已换回**正式温湿度监视器**，每秒打印一次 `Temp: xx.x C    Hum: xx.x %`。
- **`driver/AHT20/aht20.c`**：未改动，逻辑本就正确（`AHT20_Read(float *humidity, float *temperature)`，注意**湿度参数在前、温度在后**）。

**若 100 kHz 下偶发 `AHT20 read FAIL`**（内部弱上拉在高速下边沿偏缓，非逻辑回退）：
1. 首选硬件解决：SDA/SCL 各加 **4.7 kΩ 上拉到 3.3 V**；
2. 或临时软件降速：把 `I2C_ClockSpeed` 改为 `50000` 或已验证的 `20000`。

**可选增强**（本次未做）：AHT20 第 7 字节是 CRC，目前忽略；对可靠性要求高时可加 CRC-8 校验；测量等待可由固定 `Delay_ms(80)` 改为轮询状态字节 bit7，更快更稳。

---

## 七、通用经验总结

1. **先怀疑自己的代码，再怀疑硬件**：本次 4 个故障全是软件问题，"无 ACK"的假象来自扫描器自身的时序缺陷。
2. **I2C 主机健壮性三件套**：①上电做**总线恢复**（9 脉冲 + STOP）；②单字节读严格遵循"**清 ADDR 后再 STOP**"；③**不依赖主机模式 STOPF**，并保证 **ACK 状态在事务间一致**。
3. **事务之间必须等总线真正空闲**：轮询式 I2C 不能"忙就跳过"，要带超时地等 STOP 完成。
4. **分步诊断 + 逐标志位打印**是把模糊的"read FAIL"快速收敛到精确失败点的最有效手段。
5. **降速只是诊断手段，不是解决方案**：确认逻辑无误后应恢复标准速度；真正的电气裕量问题靠外部上拉解决。
