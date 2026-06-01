#include "bll_motor.h" 
#include "sys.h"
#include "modbus_master.h"

#include "jexception.h"

void Ready_Brake(void)
{
	Send_Func06_Data(POSITION_TRIGGER_REGISTER, POSITION_READY_ENABLE_CMD);
}

void Brake(void)
{
	Send_Func06_Data(POSITION_TRIGGER_REGISTER, POSITION_ENABLE_CMD);
}

void Ready_Run(void)
{
	Send_Func06_Data(POSITION_TRIGGER_REGISTER, POSITION_READY_ENABLE_CMD);
}

void Run(void)
{
	Send_Func06_Data(POSITION_TRIGGER_REGISTER, POSITION_ENABLE_CMD);
}

void Position_Mode_Set_Speed(s32 Speed)
{
	Send_Func10_Data(POSITION_MODE_SPEED_REGISTER, Speed);//位置模式设置运行速度
}

void Position_Mode_Set_Acce(s32 Acce)
{
	Send_Func10_Data(POSITION_MODE_ACCE_REGISTER, Acce);//减速度
}

void Position_Mode_Set_Dece(s32 Dece)
{
	Send_Func10_Data(POSITION_MODE_DECE_REGISTER, Dece);//减速度
}

void Set_Position(s32 Position)
{
	Send_Func10_Data(POSITION_REGISTER, Position);
}

EXTERN_EXCEPTION_CUSTOM(ex_pos);
ModBusFailCode_Type is_modbus_error = 0;

static u8 wait_flag = 0, code = 0;
static void Check_Status_Cb(ModBusFailCode_Type err,u8* data,u16 len)
{
	wait_flag = 0;
	code = data[4];
	REALTIME_REG.LastMotorState = code | ((u8)err);
}

u8 Check_Status(void)
{
	wait_flag = 1;
	Send_Func03_Data(PLACED_REGISTER, 1, Check_Status_Cb);
	while(wait_flag);
	return code;
}

static int encoder_number = 0;
static void Get_Position_Cb(ModBusFailCode_Type err,u8* data,u16 len)
{
	wait_flag = 0;
	is_modbus_error = err;
	encoder_number = data[5]<<24 | data[6]<<16 | data[3]<<8 | data[4];
}

s32 Get_Encoder_Number(void)
{
	wait_flag = 1;
	is_modbus_error = 0;
	Send_Func03_Data(ENCODER_NUMBER_REGISTER, 2, Get_Position_Cb);
	while(wait_flag);
	if(is_modbus_error != 0)
		throwb(ex_pos,is_modbus_error);
	return encoder_number;
}

static int turn_number = 0;
static void Get_Turn_Cb(ModBusFailCode_Type err,u8* data,u16 len)
{
	wait_flag = 0;
	is_modbus_error = err;
	turn_number = data[5]<<24 | data[6]<<16 | data[3]<<8 | data[4];
}

s32 Get_Turn_Number(void)
{
	wait_flag = 1;
	is_modbus_error = 0;
	Send_Func03_Data(TURN_NUMBER_REGISTER, 2, Get_Turn_Cb);
	while(wait_flag);
	if(is_modbus_error != 0)
		throwb(ex_pos,is_modbus_error);
	return turn_number;
}

void Check_Fault(void)
{
	Send_Func03_Data(FAULT_REGISTER, 1, 0);
}




