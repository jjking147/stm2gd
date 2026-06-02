#include "sys.h"
#include "string.h"
#include "usart.h"
#include "modbus_master.h"
#include "modbus_slave.h"

//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB
#if 1
#pragma import(__use_no_semihosting)
//标准库需要的支持函数
struct __FILE
{
	int handle;
};

FILE __stdout;
//定义_sys_exit()以避免使用半主机模式
void _sys_exit(int x)
{
	x = x;
}
//重定向fputc函数
int fputc(int ch, FILE *f)
{
	while (usart_flag_get(USART0, USART_FLAG_TBE) == RESET)
		; //循环发送,直到发送完毕
	usart_data_transmit(USART0, (uint8_t)ch);
	return ch;
}
#endif

// bound:波特率
void Master_USART_Init(u32 bound)
{
	/* 使能时钟 */
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_USART1);

	/* 配置TX引脚 (PA2) */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
	gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_2);

	/* 配置RX引脚 (PA3) */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_3);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
	gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_3);

	/* USART配置 */
	usart_deinit(USART1);
	usart_baudrate_set(USART1, bound);
	usart_word_length_set(USART1, USART_WL_8BIT);
	usart_stop_bit_set(USART1, USART_STB_1BIT);
	usart_parity_config(USART1, USART_PM_NONE);
	usart_hardware_flow_rts_config(USART1, USART_RTS_DISABLE);
	usart_hardware_flow_cts_config(USART1, USART_CTS_DISABLE);
	usart_receive_config(USART1, USART_RECEIVE_ENABLE);
	usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
	usart_enable(USART1);

	/* 使能接收中断 */
	usart_interrupt_enable(USART1, USART_INT_RBNE);
	nvic_irq_enable(USART1_IRQn, 3, 3);
}

void USART1_IRQHandler(void)
{
	if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE) != RESET)
	{
		usart_interrupt_flag_clear(USART1, USART_INT_FLAG_RBNE);
		Modbus_Master_Receive(usart_data_receive(USART1));
	}
}

void Slave_USART_Init(u32 bound)
{
	/* 使能时钟 */
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_USART0);

	/* 配置TX引脚 (PA9) */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_9);
	gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9);

	/* 配置RX引脚 (PA10) */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_10);
	gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_10);

	/* USART配置 */
	usart_deinit(USART0);
	usart_baudrate_set(USART0, bound);
	usart_word_length_set(USART0, USART_WL_8BIT);
	usart_stop_bit_set(USART0, USART_STB_1BIT);
	usart_parity_config(USART0, USART_PM_NONE);
	usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
	usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
	usart_receive_config(USART0, USART_RECEIVE_ENABLE);
	usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
	usart_enable(USART0);

	/* 使能接收中断 */
	usart_interrupt_enable(USART0, USART_INT_RBNE);
	nvic_irq_enable(USART0_IRQn, 0, 0);

	USART1_Tx_InitDMA();
}

static u8 DMA_Buffer[256];
static u8 DMA_Status = 0;

void USART1_Tx_InitDMA(void)
{
	/* 使能DMA时钟 */
	rcu_periph_clock_enable(RCU_DMA1);

	/* 配置DMA通道7用于USART0 TX */
	dma_deinit(DMA1, DMA_CH7);
	dma_single_data_parameter_struct dma_init_struct;
	dma_init_struct.periph_addr = (uint32_t)&USART_DATA(USART0);
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.memory0_addr = (uint32_t)DMA_Buffer;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
	dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
	dma_init_struct.number = 0;
	dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
	dma_single_data_mode_init(DMA1, DMA_CH7, &dma_init_struct);

	/* 选择DMA通道的子外设: USART0 TX 对应 DMA_SUBPERI4 */
	dma_channel_subperipheral_select(DMA1, DMA_CH7, DMA_SUBPERI4);

	/* 使能USART0 DMA发送 */
	usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);

	/* 使能DMA传输完成中断 */
	dma_interrupt_enable(DMA1, DMA_CH7, DMA_CHXCTL_FTFIE);
	nvic_irq_enable(DMA1_Channel7_IRQn, 0, 0);
}

// 发送DMA数据
void USART1_SendWithDMA(u8 *data, u32 len)
{
	while (DMA_Status == 1)
		;
	memcpy(DMA_Buffer, data, len);

	dma_channel_disable(DMA1, DMA_CH7);
	dma_transfer_number_config(DMA1, DMA_CH7, len);
	dma_channel_enable(DMA1, DMA_CH7);
	DMA_Status = 1;
}

void USART1_SendByte(u8 byte)
{
	usart_data_transmit(USART0, (uint8_t)byte);
	while (usart_flag_get(USART0, USART_FLAG_TBE) == RESET)
		;
}

void USART1_SendBytes(u8 *bytes, u32 len)
{
	while (len--)
	{
		USART1_SendByte(*bytes++);
	}
}

void USART1_SendString(u8 *str)
{
	while (*str)
	{
		USART1_SendByte(*str++);
	}
}

void USART0_IRQHandler(void)
{
	if (usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE) != RESET)
	{
		usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE);
		ModBusRecByte(usart_data_receive(USART0));
	}
}

void DMA1_Channel7_IRQHandler(void)
{
	if (dma_interrupt_flag_get(DMA1, DMA_CH7, DMA_INT_FLAG_FTF) != RESET)
	{
		dma_interrupt_flag_clear(DMA1, DMA_CH7, DMA_INT_FLAG_FTF);
		dma_flag_clear(DMA1, DMA_CH7, DMA_FLAG_FTF);
		DMA_Status = 0;
		Modbus_ResetToIdle();
	}
}

void WriteToFlash(u32 *dataArray, u16 len, u8 sector, u8 *flashAddr)
{
	fmc_unlock();
	fmc_sector_erase((uint32_t)flashAddr);
	// Loop to program all data
	for (u16 i = 0; i < len; i += 4)
	{
		fmc_word_program((uint32_t)(flashAddr + i), *(uint32_t *)(dataArray + i));
	}
	fmc_lock();
}
