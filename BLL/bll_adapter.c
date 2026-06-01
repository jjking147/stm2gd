#include "bll_adapter.h"
#include "bll_motor.h"
#include "config.h"
#include "gpio.h"
#include "delay.h"

#include "math.h"

#include "jexception.h"
#include "modbus_slave.h"

volatile s32 zero_turn_num;
volatile s32 zero_encoder_num;
vu8 Limit_Triggered = 0;

u8 Check_LimitTriggered(void)
{
	// #if USE_LIMIT_SENSOR
	//	u8 rt = Limit_Triggered;
	//	Limit_Triggered = 0;
	//	return rt;
	// #else
	//	return 0;
	// #endif
	if (XIN(6) || XIN(7)) // 如果PC6或者PC7有读到数值
	{
		delay_ms(3);			   // 消抖
		return (XIN(6) || XIN(7)); // 如果返回的还是1，说明有限位传感器触发了
	}
	else
		return 0;
}

void EStop_INT_HANDLER(u8 updown)
{
	if (1 == 1 && XIN(6) == 1)
	{
		delay_ms(1);
		if (updown == 1 && XIN(6) == 1)
		{
			Limit_Triggered = 1;
		}
	}
	else if (0 == 0 && XIN(7) == 1)
	{
		delay_ms(1);
		if (updown == 1 && XIN(6) == 1)
		{
			Limit_Triggered = 1;
		}
	}
}

int __Unimplemented()
{
	return -1;
}

int FUN_MOTOR_STOP(void)
{
	return -1;
}

int FUN_MOTOR_ABORT(void)
{
	return -1;
}

u8 BLL_Moter_AD_BackZero(s32 acce, s32 speed)
{
	Position_Mode_Set_Speed(speed);

	Position_Mode_Set_Acce(acce);

	Position_Mode_Set_Dece(acce * 100);

	Set_Position(0x0fffffff);

	Ready_Run();

	Run();

	Set_Position(0);

	Ready_Brake();

	return 0;
}

u8 BLL_Motor_AD_RelativeMove(s32 distance, s32 acce, s32 dece, s32 speed)
{
	Position_Mode_Set_Speed(speed);

	Position_Mode_Set_Acce(acce);

	Position_Mode_Set_Dece(dece);

	Set_Position(distance);

	Ready_Run();

	Run();

	Set_Position(0);

	Ready_Brake();

	return 0;
}

s32 position_current;
INIT_EXCEPTION_CUSTOM(ex_pos);
extern ModBusFailCode_Type is_modbus_error;

u8 BLL_Motor_AD_AbsoluteMove(s32 distance, s32 acce, s32 dece, s32 speed)
{
	//	tryb(ex_pos)
	//	{
	//		position_current = BLL_Motor_AD_Get_Position();
	//	}
	//	catchb(pos_ex)
	//	{
	//		return 2;
	//	}

	position_current = BLL_Motor_AD_Get_Position();
	if (is_modbus_error != MODBUS_OK)
		return 2;

	s32 move = distance - position_current;
	if (abs(move) > 100000) // 绝对位移最大限度，每1000=同步轮转一圈，同步轮半径大约2.3cm
		return 1;
	BLL_Motor_AD_RelativeMove(move, acce, dece, speed);

	return 0;
}

u8 BLL_Motor_AD_SpeedMode(s32 acce, s32 dece, s32 speed, u8 dir)
{
	Position_Mode_Set_Speed(speed);

	Position_Mode_Set_Acce(acce);

	Position_Mode_Set_Dece(dece);

	if (dir)
	{
		Set_Position(-0x0fffffff); // 向上
	}
	else
	{
		Set_Position(0x0fffffff); // 向下
	}

	Ready_Run();

	Run();

	Set_Position(0);

	Ready_Brake();

	return 0;
}

s32 BLL_Motor_AD_Get_Position(void)
{
	double position;
	s32 turn_num, encoder_num;

	turn_num = Get_Turn_Number();

	turn_num -= zero_turn_num;

	encoder_num = Get_Encoder_Number();

	encoder_num -= zero_encoder_num;

	position = CONVERT_POSITION(encoder_num, turn_num);

	return (s32)position;
}
