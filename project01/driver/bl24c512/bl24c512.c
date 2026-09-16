#include "bl24c512.h"
#include "i2c.h"
#include "stm32f4xx.h"
#include "stdbool.h"

#define BL24C512_PAGE_SIZE 128
#define BL24C512_ADDRESS 0xA0

bool BL24C512_PageWrite(uint16_t address, uint8_t data[], uint32_t length)
{
	i2c_t *bus = &i2c1_bus;                       /* 本 EEPROM 挂在哪一路 I2C */
	I2C_TypeDef *I2Cx = bus->I2Cx;

	/* 发送 START，等待总线空闲并产生起始条件 */
	if(!i2c_wait_flag(bus, I2C_FLAG_BUSY, RESET)) return false;    // 等待总线空闲（上次写周期结束）
	I2C_GenerateSTART(I2Cx, ENABLE);
	if(!i2c_wait_flag(bus, I2C_FLAG_SB, SET)) return false;    // 等待 SB 置位（起始条件已发送）

	/* 发送设备地址（写方向），等待从机应答 */
	I2C_Send7bitAddress(I2Cx, BL24C512_ADDRESS, I2C_Direction_Transmitter);
	if(!i2c_wait_flag(bus, I2C_FLAG_ADDR, SET)) return false;  // 等待 ADDR (地址匹配标志) 置位（从机已应答）
	/*
	关键机制——时钟拉伸：当 ADDR=1 且软件还没清除它时，I2C 外设会把 SCL 拉低（时钟拉伸），
	让整个总线暂停，等软件处理。只有清掉 ADDR，外设才释放 SCL，后续数据传输才能继续。
	*/
	(void)I2Cx->SR2;                                       // 前面读了SR1 现在再读 SR2 清除 ADDR，启动数据发送（兼确认写周期结束）

	/* 发送 16 位存储地址 */
	I2C_SendData(I2Cx, (address >> 8) & 0xFF);
	if(!i2c_wait_flag(bus, I2C_FLAG_TXE, SET)) return false;
	I2C_SendData(I2Cx, address & 0xFF);
	if(!i2c_wait_flag(bus, I2C_FLAG_TXE, SET)) return false;

	/* 逐字节发送数据 */
	for(uint32_t i = 0; i < length; i++)
	{
		I2C_SendData(I2Cx, data[i]);
		if(!i2c_wait_flag(bus, I2C_FLAG_TXE, SET)) return false;
	}

	/*
	原来等 TXE 只代表数据寄存器空，最后一个字节可能还在移位寄存器里往外发，此时发 STOP 会截断数据
	等待最后一字节移出移位寄存器（BTF），再发 STOP
	BTF（Byte Transfer Finished）才表示该字节连同应答已完整传输。
	*/
	if(!i2c_wait_flag(bus, I2C_FLAG_BTF, SET)) return false; //
	I2C_GenerateSTOP(I2Cx, ENABLE);
	if(!i2c_wait_flag(bus, I2C_FLAG_STOPF, SET)) return false; // 等待 STOP 发送完成，总线释放
	return true;
}

bool BL24C512_write(uint16_t address, uint8_t data[], uint32_t length)
{
	while(length > 0)
	{
		/* 本轮最多写到当前页末尾，防止跨页写入导致地址回卷 */
		uint32_t chunk = BL24C512_PAGE_SIZE - (address % BL24C512_PAGE_SIZE);
		if(chunk > length) chunk = length;
		if(!BL24C512_PageWrite(address, data, chunk)) return false;
		address += chunk;
		data    += chunk;
		length  -= chunk;
	}
	return true;
}

bool BL24C512_read(uint16_t address, uint8_t data[], uint32_t length)
{
	i2c_t *bus = &i2c1_bus;                       /* 本 EEPROM 挂在哪一路 I2C */
	I2C_TypeDef *I2Cx = bus->I2Cx;

	if(length == 0) return true;                                    // 无需读取，直接成功（避免后续等不到 STOPF）

	/* 阶段一：哑写，设置芯片内部地址指针 */
	if(!i2c_wait_flag(bus, I2C_FLAG_BUSY, RESET)) return false;    // 等待总线空闲（上次写周期结束）
	I2C_GenerateSTART(I2Cx, ENABLE);
	if(!i2c_wait_flag(bus, I2C_FLAG_SB, SET)) return false;        // 等待 SB 置位（起始条件已发送）
	I2C_Send7bitAddress(I2Cx, BL24C512_ADDRESS, I2C_Direction_Transmitter);
	if(!i2c_wait_flag(bus, I2C_FLAG_ADDR, SET)) return false;      // 等待 ADDR 置位（从机已应答）
	(void)I2Cx->SR2;                                                // 读 SR2 清除 ADDR，启动地址发送（兼确认写周期结束）
	I2C_SendData(I2Cx, (address >> 8) & 0xFF);
	if(!i2c_wait_flag(bus, I2C_FLAG_TXE, SET)) return false;
	I2C_SendData(I2Cx, address & 0xFF);
	if(!i2c_wait_flag(bus, I2C_FLAG_TXE, SET)) return false;

	/* 阶段二：重发 START，进入接收模式 */
	I2C_AcknowledgeConfig(I2Cx, ENABLE);
	I2C_GenerateSTART(I2Cx, ENABLE);                                // 重发 START（不发 STOP，地址指针保持）
	if(!i2c_wait_flag(bus, I2C_FLAG_SB, SET)) return false;
	I2C_Send7bitAddress(I2Cx, BL24C512_ADDRESS, I2C_Direction_Receiver);
	if(!i2c_wait_flag(bus, I2C_FLAG_ADDR, SET)) return false;

	/*
	每个字节的第 9 个时钟是发 ACK/NACK 的固定时刻，也是配置的截止线；
	单字节时一清 ADDR 时钟就立刻开跑，只能借用清 ADDR 前时钟拉伸的无限窗口提前把 ACK=0 写好。
	*/
	if(length == 1)                                                 // 单字节：清 ADDR 前关 ACK 并发 STOP（手册规定的时序要求）
	{
		I2C_AcknowledgeConfig(I2Cx, DISABLE);
		I2C_GenerateSTOP(I2Cx, ENABLE);
	}
	(void)I2Cx->SR2;                                                // 读 SR2 清除 ADDR，释放 SCL，开始接收数据（兼确认地址匹配）

	/*
	读操作无页边界限制：芯片内部地址计数器可跨页连续递增不回卷，
	因此任意长度一次读完，不需要像写那样分块循环。
	*/
	for(uint32_t i = 0; i < length; i++)
	{
		if(i == length - 1 && length > 1)                           // 最后一字节：关 ACK + STOP，通知从机停止发送（兼确认字节接收完成）
		{
			I2C_AcknowledgeConfig(I2Cx, DISABLE);
			I2C_GenerateSTOP(I2Cx, ENABLE);
		}
		if(!i2c_wait_flag(bus, I2C_FLAG_RXNE, SET)) return false;  // 先等数据就绪（带超时与 NACK 保护）
		data[i] = I2C_ReceiveData(I2Cx);                            // 再读数据寄存器（兼清 RXNE）
	}
	if(!i2c_wait_flag(bus, I2C_FLAG_STOPF, SET)) return false;     // 等待 STOP 发送完成，总线释放（兼确认总线空闲）
	I2C_AcknowledgeConfig(I2Cx, ENABLE);                            // 恢复 ACK，供下次使用（兼确认外设状态）
	return true;
}
