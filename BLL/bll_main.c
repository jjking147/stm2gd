#include "bll_main.h"
#include "timer.h"
#include "usart.h"
#include "gpio.h"
#include "exti.h"
#include "iwdg.h"
#include "string.h"
#include "modbus_slave.h"
#include "bll_adapter.h"
#include "bll_eeprom.h"
#include "bll_backzero.h"
#include "bll_tocase.h"
#include "config.h"

Trigger_Type Current_Trigger = TRIG_None;
ParamShadow_Type ParamShadow;

void BLL_Init_All(void)
{
	// NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	Slave_USART_Init(MODBUS_BAUDRATE);
	Modbus_Slave_Timer_Init(MOBUS_SLAVE_TIMEOUT);
	Master_USART_Init(MOTOR_BAUDRATE);
	Modbus_Master_Timer_Init(MOBUS_MASTER_TIMEOUT);
	Delay_Timer_Init();
	Dispatcher_Tim_Init(DISPATCHER_TIME_SPAN);
	GPIO_Config();
	//	IWDG_Init(4,5000);
	NVIC_Config(1, 0);
	NVIC_Config(2, 0);
	//	NVIC_Config(3,0);
	NVIC_Config(4, 0);
	NVIC_Config(5, 0);
#if USE_LIMIT_SENSOR
	// NVIC_Config(6,0);
	// NVIC_Config(7,0);
#endif
	System_State = SYS_Idel;
	System_Inited = 1;
}

u8 ExecuteCommand_Handler(u16 *cmd)
{
	for (u8 i = 0; i < 8; i++)
	{
		if (cmd[i] == 0xA5A5)
		{
			Current_Trigger = (Trigger_Type)i; // 获取cmd中0xA5A5的索引，并且转换为Trigger信号
			break;
		}
	}
	memset(cmd, 0, 16);											// 找到之后直接清零
	memcpy(&ParamShadow, &PARAM_REG, sizeof(ParamShadow_Type)); // 配置这个任务的参数

	if (Current_Trigger == TRIG_MissionStart) // 一些需要判断并且回滚的情况
	{
		if (System_State == SYS_Faliure) // 任务开始但是系统失败直接返回
		{
			return MODBUS_ERR_Failure;
		}
		if (System_State == SYS_Working || System_State == SYS_Finished) // 任务开始但是系统状态不是空闲，直接返回
		{
			return MODBUS_ERR_Busy;
		}
	}
	if (Current_Trigger == TRIG_ClearFinishFlag && System_State != SYS_Finished)
	{
		return MODBUS_ERR_Busy;
	}
	if (Current_Trigger == TRIG_ClearFailure && System_State != SYS_Faliure)
	{
		return MODBUS_ERR_Busy;
	}

	System_State = SYS_Working; // 系统状态设置为正在工作
	return MODBUS_OK;
}

void BLL_SetFailure(FailureID_Type failure)
{
	System_State = SYS_Faliure;
	System_Failure = failure;
}

void BLL_ClearFailure(void)
{
	System_State = SYS_Idel;
	System_Failure = Failure_None;
	Current_Trigger = TRIG_None;
}

void BLL_ClearFinishFlag(void)
{
	PARAM_REG.State.SystemState = SYS_Idel;
	Current_Trigger = TRIG_None;
}

void BLL_Stop_Mission(void) // 可能是升降这里不需要
{
	u8 flag = FUN_MOTOR_STOP();		   // 这个函数直接返回-1
	if (flag == ADAPTER_PLACEHOLD_VAL) // 何意味
	{
		FUN_MOTOR_CLEAR_RX_FLAG();
		System_State = SYS_Finished;
		Current_Trigger = TRIG_None;
	}
}

void BLL_Abort_Mission(void) // 何意味，可能是升降这里不需要？
{
	u8 flag = FUN_MOTOR_ABORT();
	if (flag == ADAPTER_PLACEHOLD_VAL)
	{
		FUN_MOTOR_CLEAR_RX_FLAG();
		System_State = SYS_Faliure;
		System_Failure = Failure_EStop;
		Current_Trigger = TRIG_None;
	}
}

void BLL_Soft_Reset(void)
{
	__set_FAULTMASK(1);
	NVIC_SystemReset();
}

/* jump to user app */
typedef void (*iapfun)(void);
iapfun jump_to_app;
uint32_t app_addr = 0x080e0000;

void BLL_Update(void)
{
	/* disable nvic irq and clear pending */
	__disable_irq();

	jump_to_app = (iapfun) * (uint32_t *)(app_addr + 4); /* code second word is reset address */
	__set_MSP(*(uint32_t *)app_addr);					 /* init app stack pointer(code first word is stack address) */
	jump_to_app();
}

static void NOP(void)
{
	__nop();
}

void BLL_Motor_Mission(void)
{
	u8 flag, err = 0;
	void (*clear_flag_fun)(void);
	switch (ParamShadow.MissionID)
	{
	case Misson_EEROM:
		flag = BLL_EEPROM_Execute(ParamShadow, &err);
		clear_flag_fun = BLL_EEPROM_ClearFlag;
		break;
	case Misson_Zero:
		flag = BLL_BackZero_Execute(ParamShadow, &err);
		clear_flag_fun = BLL_BackZero_ClearFlag;
		break;
	case Misson_Move:
		flag = BLL_ToCase_Execute(ParamShadow, &err);
		clear_flag_fun = BLL_ToCase_ClearFlag;
		break;
	case Mission_Claw:
		// 夹爪/伸缩任务
		break;
	case Mission_ForwardData:
		// 转发数据任务
		break;
	default:
		flag = CSF_Finished;
		err = Failure_Unsupported;
		clear_flag_fun = NOP;
		break;
	}
	if (flag == CSF_Finished)
	{
		clear_flag_fun();
		if (err == 0)
			System_State = SYS_Finished;
		else
		{
			System_State = SYS_Faliure;
			System_Failure = (FailureID_Type)err;
		}
		Current_Trigger = TRIG_None;
	}
}

void BLL_Execute_Mission(void)
{
	if (System_State == SYS_Working)
	{
		switch (Current_Trigger)
		{
		case TRIG_MissionStart:
			BLL_Motor_Mission();
			break;
		case TRIG_ClearFinishFlag:
			BLL_ClearFinishFlag();
			break;
		case TRIG_ClearFailure:
			BLL_ClearFailure();
			break;
		case TRIG_StopMission:
			BLL_Stop_Mission();
			break;
		case TRIG_AbortMission:
			BLL_Abort_Mission();
			break;
		case TRIG_SoftReset:
			BLL_Soft_Reset();
			break;
		default:
			break;
		}
	}
}
