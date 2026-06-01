#include "bll_backzero.h"
#include "bll_adapter.h"
#include "bll_motor.h"
#include "modbus_master.h"
#include "config.h"
#include "delay.h"
#include "usart.h"
#include "timer.h"
#include "gpio.h"
#include "jexception.h"
#include "exti.h"

extern volatile u8 has_zero_flag;
extern volatile u8 last_target;
extern volatile vu8 swtich4_count;
volatile u8 wait_zero_flag;
volatile u8 zero_trigger_flag;
volatile u32 zero_trigger_time = 0xffffffff;
extern volatile u8 running_flag;
extern volatile s32 zero_turn_num;
extern volatile s32 zero_encoder_num;
extern volatile ModBusState Master_State;

static CommonStateFlag_Type backzero_flag = CSF_Idel;//声明原来的状态

void BLL_BackZero_ClearFlag(void)
{
	backzero_flag = CSF_Idel;
}

//#define WAIT_MOTOR_STOP(span,n,label)	{ \
//	u16 _motor_sate = 0,_retry = 0; \
//	do \
//	{ \
//		if(_retry++ >= n) \
//		{ \
//			*err = Failure_Timeout; \
//			goto label; \
//		} \
//		delay_ms(span); \
//		_motor_sate = Check_Status(); \
//	}while((_motor_sate & 0x01) != 0x01); \
//}

static void WaitMotorStop(u16 span,u16 n)
{
	u16 _motor_sate = 0,_retry = 0; 
	do 
	{ 
		delay_ms(span); 
		if(_retry++ >= n) 
		{ 
			throw(Failure_Timeout);
		} 
		if(Check_LimitTriggered())
		{
			Brake();
			throw(Failure_Limit);
		}
		
		_motor_sate = Check_Status(); 
	}while((_motor_sate & 0x01) != 0x01); 

}

static void ZeroMove(u8 dir,u32 acc,u32 dece,u32 speed,s32 maxlen)
{
	if(Read_Switch(5) == RESET)
	{
		wait_zero_flag = 1;
		zero_trigger_flag = 0;
		u32 start = ReadTick();
		
		
		//BLL_Motor_AD_SpeedMode(acc,dece,speed,dir);
		BLL_Motor_AD_RelativeMove(maxlen,acc,dece,speed);
		
		while(wait_zero_flag) 
		{		
			if(zero_trigger_flag)
			{
				if(XIN(5) == SET)
				{
					if(TickSpan(zero_trigger_time) > 4)
					{
						USART_ClearFlag(MOTOR_USART, USART_FLAG_TC);
						Master_State = MODBUS_IDLE;
						Brake();
						delay_ms(100);
						WaitMotorStop(200,2000);	//可能是200ms查询间隔碰巧可行，不代表过冲时电机到位标识真就可信
						zero_trigger_flag = 0;
						wait_zero_flag = 0;
						break;
					}
				}
				else
				{
					zero_trigger_flag = 0;
				}
			}
			if(TickSpan(start) > 20000)
			{
				USART_ClearFlag(MOTOR_USART, USART_FLAG_TC);
				Master_State = MODBUS_IDLE;
				Brake();
				delay_ms(100);
				WaitMotorStop(200,2000);	//可能是200ms查询间隔碰巧可行，不代表过冲时电机到位标识真就可信
				zero_trigger_flag = 0;
				wait_zero_flag = 0;
				throw(Failure_Zero);
			}
		}
	}
}

void BackZero(void)
{
	ZeroMove(0,15000,150000,20000,140000);	//向下运动 15000-20000
	delay_ms(500);
	ZeroMove(1,5000,50000,2000,-5000);	//向上运动
	if(Read_Switch(5) == RESET)
	{
		throw(Failure_Zero);
	}
	BLL_Motor_AD_UpdateZero();
	has_zero_flag = 1;
	last_target = 0;
	swtich4_count = 0;
}

CommonStateFlag_Type BLL_BackZero_Execute(ParamShadow_Type params, u8 *err)
{
	if(backzero_flag == CSF_Idel)
	{
		//Step0：初始化各种标志位等
		backzero_flag = CSF_Working;
		running_flag = 1;
		USART_ClearFlag(MOTOR_USART, USART_FLAG_TC);	
		
		//Step1：进行回零
		try
		{
			BackZero();
		}
		catch
		{
			*err = GetLastError();
			goto die;
		}
	}
	
//最终程序出口
die:	
	backzero_flag = CSF_Finished;
	running_flag = 0;
	return backzero_flag;
}

void SWTICH5_INT_HANDLER(void)
{
	if((running_flag == 1) && (wait_zero_flag == 1) && (zero_trigger_flag == 0)) 
	{
		delay_us(10);
		if(XIN(5) == SET)
		{
			zero_trigger_time = ReadTick();
			zero_trigger_flag = 1;
		}
		
	}
}
