#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "timer.h"
#include "modbus_master.h"
#include "modbus_slave.h"

// Modbus主机的函数文件，主要流程就是主机在空闲状态，发送03,06,10指令(用modbus协议进行包装)给电机，
// 然后通过电机返回的信号来触发USART2端口的中断，进入中断函数中的读信号流程，读信号流程呢又会启动TIM7定时器进行定时，
// 读信号流程中会不断清理TIM7定时器的计数值，如果计数值溢出触发中断了就说明已经没有信号发送过来了，可以停止。
// 计数值溢出触发中断的情况下，Modbus主机的状态会被设置为TIMEOUT，如果没有被设置为TIMEOUT太久也会触发错误回归的逻辑
extern u8 MOTOR_ADDRESS;

volatile ModBusState Master_State = MODBUS_IDLE;
static volatile u16 Master_Rec_Count = 0;
u8 Master_Receive_Buff[MODBUS_MASTER_BUFF_LEN];
u16 USART_RX_STA = 0;

void Master_Set_Idle(void)
{
	Master_State = MODBUS_IDLE;
}

void Master_Set_Timeout(void)
{
	Master_State = MODBUS_TIMEOUT;
}

void Modbus_Master_Receive(u8 data) // Modbus主机接受信号函数，接受一个字节之后打开TIM7定时器，然后就开始不断接受信号
{
	if (Master_State == MODBUS_WAIT_RESPONSE)
	{
		Master_Receive_Buff[0] = data; // 总共能装200个字节
		Master_Rec_Count = 1;
		TIM_Cmd(MODBUS_MASTER_TIM, ENABLE); // 用的TIM7
		TIM_SetCounter(MODBUS_MASTER_TIM, 0);
		Master_State = MODBUS_RECING;
	}
	else if (Master_State == MODBUS_RECING)
	{
		Master_Receive_Buff[Master_Rec_Count++] = data;
		if (Master_Rec_Count >= MODBUS_BUFF_LEN)
		{
			Master_Rec_Count = 0;
			TIM_SetCounter(MODBUS_MASTER_TIM, 0); // 定时清理一下TIM的计数值，防止中断停止接受信号了
		}
	}
}

void Modbus_Master_SendUART(u8 *data, u16 crc_result, u16 len) // Modbus_Master主机发送串口通信，先发data，再发CRC
{
	for (u8 i = 0; i < len; i++)
	{
		USART_SendData(MOTOR_USART, data[i]);
		while (USART_GetFlagStatus(MOTOR_USART, USART_FLAG_TC) != SET)
			;
	}
	USART_SendData(MOTOR_USART, crc_result); // 发送低9位
	while (USART_GetFlagStatus(MOTOR_USART, USART_FLAG_TC) != SET)
		;
	crc_result = crc_result >> 8; // 发送高8位
	USART_SendData(MOTOR_USART, crc_result);
	while (USART_GetFlagStatus(MOTOR_USART, USART_FLAG_TC) != SET)
		;
}

// 主机发送数据，
void Send_Func03_Data(u16 Reg, u16 Num, MODBUS_RESPONSE_CALLBACK fun) // 发送03请求，读寄存器
{
	u8 USART_TX_BUF[6];
	u16 crc_result;

	if (Master_State == MODBUS_IDLE)
	{
		Master_State = MODBUS_SENDING;
		USART_TX_BUF[0] = MOTOR_ADDRESS;
		USART_TX_BUF[1] = 0x03;				 // 读的指令
		USART_TX_BUF[2] = (Reg >> 8) & 0xFF; // 先发高八位
		USART_TX_BUF[3] = Reg & 0xFF;		 // 再发低八位
		USART_TX_BUF[4] = (Num >> 8) & 0xFF; // 先发高八位
		USART_TX_BUF[5] = Num & 0xFF;		 // 再发低八位

		crc_result = Calculate_CRC(USART_TX_BUF, 6); // 计算这个USART_TX_BUF独属的16位CRC值

		Modbus_Master_SendUART(USART_TX_BUF, crc_result, 6); // 先把USART_TX_BUF里的数据发完，再按高八位低八位的顺序发CRC，这里就是在发送请求了

		Master_State = MODBUS_WAIT_RESPONSE; // 改成MODBUS_WAIT_RESPONSE,方便后续USART2_IRQHandler中断处理函数中的Modbus_Master_Receive

		u32 start = ReadTick();

		// 其实这里的MODBUS_TIMEOUT是由TIM7进入中断主动处理的，说明已经有一段时间没有接受到数据了，触发超时回归
		while (Master_State != MODBUS_TIMEOUT) // 阻塞一直到Master_state=MODBUS_TIMEOUT,然后查看Master_Receive_Buff[1]的第八位，是1的话直接返回错误
		{
			if (TickSpan(start) > MODBUS_MASTER_LOSS_TIME)
			{
				if (fun != 0)
				{
					Master_State = MODBUS_HANDLING;
					(*fun)(MODBUS_ERR_NoResponse, 0, 0);
				}
				Master_State = MODBUS_IDLE;
				return;
			}
		}

		if (Master_Receive_Buff[1] & 0x80)
		{
			if (fun != 0)
			{
				Master_State = MODBUS_HANDLING;
				(*fun)(MODBUS_ERR_Failure, 0, 0);
			}
			Master_State = MODBUS_IDLE;
			return;
		}

		if (fun != 0)
		{
			Master_State = MODBUS_HANDLING;
			(*fun)(MODBUS_OK, Master_Receive_Buff, Master_Rec_Count);
			Master_State = MODBUS_IDLE;
		}
		Master_State = MODBUS_IDLE;
	}
}

ModBusFailCode_Type Send_Func06_Data(u16 Reg, u16 Cmd) // 发送06请求，写寄存器，对应地址写一个16位的数据
{
	u8 USART_TX_BUF[6];
	u16 crc_result;

	if (Master_State == MODBUS_IDLE)
	{
		Master_State = MODBUS_SENDING;
		USART_TX_BUF[0] = MOTOR_ADDRESS;
		USART_TX_BUF[1] = 0x06; // 发送06请求
		USART_TX_BUF[2] = (Reg >> 8) & 0xFF;
		USART_TX_BUF[3] = Reg & 0xFF;
		USART_TX_BUF[4] = (Cmd >> 8) & 0xFF;
		USART_TX_BUF[5] = Cmd & 0xFF;

		crc_result = Calculate_CRC(USART_TX_BUF, 6);

		Modbus_Master_SendUART(USART_TX_BUF, crc_result, 6); // 这里是正式发送请求了

		Master_State = MODBUS_WAIT_RESPONSE;

		u32 start = ReadTick();
		while (Master_State != MODBUS_TIMEOUT)
		{
			if (TickSpan(start) > MODBUS_MASTER_LOSS_TIME)
			{
				Master_State = MODBUS_IDLE;
				return MODBUS_ERR_NoResponse;
			}
		}

		if (Master_Receive_Buff[1] & 0x80)
		{
			Master_State = MODBUS_IDLE;
			return MODBUS_ERR_Failure;
		}
		Master_State = MODBUS_IDLE;
		return MODBUS_OK;
	}
	return MODBUS_ERR_Busy;
}

ModBusFailCode_Type Send_Func10_Data(u16 Reg, u32 Cmd) // 发送10请求，连续写一段地址
{
	u8 USART_TX_BUF[11];
	u16 crc_result;

	if (Master_State == MODBUS_IDLE)
	{
		Master_State = MODBUS_SENDING;
		USART_TX_BUF[0] = MOTOR_ADDRESS;
		USART_TX_BUF[1] = 0x10;
		USART_TX_BUF[2] = (Reg >> 8) & 0xFF;
		USART_TX_BUF[3] = Reg & 0xFF;
		USART_TX_BUF[4] = 0x00;
		USART_TX_BUF[5] = 0x02;
		USART_TX_BUF[6] = 0x04;
		USART_TX_BUF[7] = (Cmd >> 8) & 0xFF;
		USART_TX_BUF[8] = Cmd & 0xFF;
		USART_TX_BUF[9] = (Cmd >> 24) & 0xFF;
		USART_TX_BUF[10] = (Cmd >> 16) & 0xFF;

		crc_result = Calculate_CRC(USART_TX_BUF, 11);

		Modbus_Master_SendUART(USART_TX_BUF, crc_result, 11); // 正式发送请求

		Master_State = MODBUS_WAIT_RESPONSE;

		u32 start = ReadTick();
		while (Master_State != MODBUS_TIMEOUT)
		{
			if (TickSpan(start) > MODBUS_MASTER_LOSS_TIME)
			{
				Master_State = MODBUS_IDLE;
				return MODBUS_ERR_NoResponse;
			}
		}

		if (Master_Receive_Buff[1] & 0x80) // 异常码
		{
			Master_State = MODBUS_IDLE;
			return MODBUS_ERR_Failure;
		}
		Master_State = MODBUS_IDLE;
		return MODBUS_OK;
	}
	return MODBUS_ERR_Busy;
}

u16 Calculate_CRC(const u8 *buffer, u16 length) // 计算CRC的函数
{
	u16 crc = 0xffff;
	uint16_t pos;
	u8 i;

	for (pos = 0; pos < length; pos++)
	{
		crc ^= (uint16_t)buffer[pos];

		for (i = 8; i != 0; i--)
		{
			if ((crc & 0x0001) != 0)
			{
				crc >>= 1;
				crc ^= 0xa001;
			}
			else
			{
				crc >>= 1;
			}
		}
	}
	return crc;
}
