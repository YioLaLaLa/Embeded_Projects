#include "i2c.h"
#include "stm32f4xx.h"
#include "stdbool.h"

/* 简单软件延时（约几 µs），仅用于总线恢复，保持 i2c.c 不依赖 delay 模块 */
static void i2c_recovery_delay(void)
{
	volatile uint32_t i = 300;
	while(i--);
}

/*
I2C 总线恢复：解除从机把 SDA 拉死导致的 BUSY 锁死。
把 SCL/SDA 临时当开漏 GPIO，手动打最多 9 个 SCL 脉冲，让卡在事务中间的从机
把残留数据位时钟完、释放 SDA，最后补一个 STOP，使总线回到空闲。
必须在把引脚配成 I2C 复用之前调用。
*/
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

    /*
    补 STOP：SDA 低
    SCL 高
    SDA 由低变高 = STOP
    */
	GPIO_ResetBits(GPIOB, GPIO_Pin_7);              // 补 STOP：SDA 低
	i2c_recovery_delay();
	GPIO_SetBits(GPIOB, GPIO_Pin_6);                // SCL 高
	i2c_recovery_delay();
	GPIO_SetBits(GPIOB, GPIO_Pin_7);                // SDA 高 = STOP
	i2c_recovery_delay();
}

/*
I2C1 总线初始化：I2C 标准模式 100kHz，7 位地址，使能 ACK；上电先做一次总线恢复
引脚：PB6 = SCL（复用 AF4），PB7 = SDA（复用 AF4），开漏 + 上拉
*/
void I2C1_Init(void)
{
	/* 开时钟：I2C1 挂在 APB1，GPIOB 挂在 AHB1 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	I2C1_BusRecovery();                             // 先恢复总线：解除上电/复位时从机可能拉死 SDA 的情况

	/* 配置 PB6/PB7 为复用开漏输出，带上拉（I2C 总线要求） */
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_StructInit(&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);

	/* 配置 I2C1 参数并使能 */
	I2C_InitTypeDef I2C_InitStruct;
	I2C_StructInit(&I2C_InitStruct);
	I2C_InitStruct.I2C_ClockSpeed = 100000;  // 标准模式 100kHz（之前的失败全是逻辑 bug，已修复）；若上拉偏弱致 100kHz 不稳定，可降到 50000 或外接 4.7kΩ 上拉
	I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStruct.I2C_OwnAddress1 = 0x00;
	I2C_InitStruct.I2C_Ack = I2C_Ack_Enable; // Enable ACKnowledgment
	I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // 7-bit acknowledged address
	I2C_Init(I2C1, &I2C_InitStruct);
	I2C_Cmd(I2C1, ENABLE);
}

/* 等待标志达到目标状态：超时或从机不应答则恢复总线并返回 false */
bool i2c_wait_flag(I2C_TypeDef *I2Cx, uint32_t flag, FlagStatus target)
{
	uint32_t timeout = 100000;
	while(I2C_GetFlagStatus(I2Cx, flag) != target)          // 还没达到目标状态 → 继续等
	{
		if(I2C_GetFlagStatus(I2Cx, I2C_FLAG_AF) == SET)     //AF:Acknowledge Failure（应答失败标志，SR1 的 bit10）
		{
			I2C_ClearFlag(I2Cx, I2C_FLAG_AF);               // 清除 NACK 标志
			I2C_GenerateSTOP(I2Cx, ENABLE);
			return false;
		}
		if(--timeout == 0)
		{
			I2C_GenerateSTOP(I2Cx, ENABLE);
			return false;
		}
	}
	return true;
}

/* 通用 I2C 写：START → 地址(写) → 逐字节发送 → STOP */
bool i2c_write(I2C_TypeDef *I2Cx, uint8_t dev_addr, const uint8_t *data, uint32_t len)
{
	if(!i2c_wait_flag(I2Cx, I2C_FLAG_BUSY, RESET)) return false;   // 等总线空闲
	I2C_AcknowledgeConfig(I2Cx, ENABLE);                          // 确保 ACK 使能：防止上次读把它关掉导致地址被误判为 NACK
	I2C_GenerateSTART(I2Cx, ENABLE);
	if(!i2c_wait_flag(I2Cx, I2C_FLAG_SB, SET)) return false;       // 起始条件已发

	I2C_Send7bitAddress(I2Cx, dev_addr, I2C_Direction_Transmitter);
	if(!i2c_wait_flag(I2Cx, I2C_FLAG_ADDR, SET)) return false;     // 从机应答
	(void)I2Cx->SR2;                                              // 清 ADDR，释放 SCL

	for(uint32_t i = 0; i < len; i++)
	{
		I2C_SendData(I2Cx, data[i]);
		if(!i2c_wait_flag(I2Cx, I2C_FLAG_TXE, SET)) return false;
	}
	if(!i2c_wait_flag(I2Cx, I2C_FLAG_BTF, SET)) return false;      // 最后一字节完整移出再 STOP
	I2C_GenerateSTOP(I2Cx, ENABLE);
	return true;
}

/* 通用 I2C 读：START → 地址(读) → 逐字节接收 → STOP */
bool i2c_read(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t *buf, uint32_t len)
{
	if(len == 0) return true;                                     // 无需读，直接成功
	if(!i2c_wait_flag(I2Cx, I2C_FLAG_BUSY, RESET)) return false;
	I2C_AcknowledgeConfig(I2Cx, ENABLE);                          // 多字节需 ACK
	I2C_GenerateSTART(I2Cx, ENABLE);
	if(!i2c_wait_flag(I2Cx, I2C_FLAG_SB, SET)) return false;

	I2C_Send7bitAddress(I2Cx, dev_addr, I2C_Direction_Receiver);
	if(!i2c_wait_flag(I2Cx, I2C_FLAG_ADDR, SET)) return false;

	if(len == 1)                                                  // 单字节 N=1（RM0090 时序）
	{
		I2C_AcknowledgeConfig(I2Cx, DISABLE);                     // 1) 先关 ACK，NACK 这唯一字节
		(void)I2Cx->SR1;                                          // 2) 读 SR1
		(void)I2Cx->SR2;                                          //    紧接读 SR2 → 可靠清 ADDR
		I2C_GenerateSTOP(I2Cx, ENABLE);                           // 3) 清 ADDR 之后再 STOP（关键：STOP 早于清 ADDR 会导致收不到字节）
	}
	else
	{
		(void)I2Cx->SR1;                                          // 多字节：显式读 SR1 ...
		(void)I2Cx->SR2;                                          // ... 紧接读 SR2 → 清 ADDR，开始接收（不再隐式依赖 143 行 wait_flag 内那次 SR1 读）
	}

	for(uint32_t i = 0; i < len; i++)
	{
		if(i == len - 1 && len > 1)                              // 多字节最后一字节：关 ACK + STOP
		{
			I2C_AcknowledgeConfig(I2Cx, DISABLE);
			I2C_GenerateSTOP(I2Cx, ENABLE);
		}
		if(!i2c_wait_flag(I2Cx, I2C_FLAG_RXNE, SET)) return false; // 等数据就绪（带超时与 NACK 保护）
		buf[i] = I2C_ReceiveData(I2Cx);                          // 读数据寄存器（兼清 RXNE）
	}
	/* 主机模式下 STOPF 不可靠（实测等不到）：STOP 已发出会自行完成，
	   由下一次传输开头的 BUSY 等待确认总线释放，此处不再等 STOPF */
	I2C_AcknowledgeConfig(I2Cx, ENABLE);                          // 恢复 ACK，供下次使用
	return true;
}
