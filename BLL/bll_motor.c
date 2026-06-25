#include "bll_motor.h"
#include "sys.h"
#include "modbus_master.h"
#include <stdio.h>

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
	Send_Func10_Data(POSITION_MODE_SPEED_REGISTER, Speed);//λ��ģʽ���������ٶ�
}

void Position_Mode_Set_Acce(s32 Acce)
{
	Send_Func10_Data(POSITION_MODE_ACCE_REGISTER, Acce);//���ٶ�
}

void Position_Mode_Set_Dece(s32 Dece)
{
	Send_Func10_Data(POSITION_MODE_DECE_REGISTER, Dece);//���ٶ�
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
	if(err == MODBUS_OK)
		encoder_number = data[5]<<24 | data[6]<<16 | data[3]<<8 | data[4];
}

s32 Get_Encoder_Number(void)
{
	for(u8 retry = 0; retry < 10; retry++)
	{
		wait_flag = 1;
		is_modbus_error = 0;
		Send_Func03_Data(ENCODER_NUMBER_REGISTER, 2, Get_Position_Cb);
		while(wait_flag);
		if(is_modbus_error == MODBUS_OK)
			return encoder_number;
	}
	throwb(ex_pos, is_modbus_error);
	return encoder_number;
}

static int turn_number = 0;
static void Get_Turn_Cb(ModBusFailCode_Type err,u8* data,u16 len)
{
	wait_flag = 0;
	is_modbus_error = err;
	if(err == MODBUS_OK)
		turn_number = data[5]<<24 | data[6]<<16 | data[3]<<8 | data[4];
}

s32 Get_Turn_Number(void)
{
	for(u8 retry = 0; retry < 10; retry++)
	{
		wait_flag = 1;
		is_modbus_error = 0;
		Send_Func03_Data(TURN_NUMBER_REGISTER, 2, Get_Turn_Cb);
		while(wait_flag);
		if(is_modbus_error == MODBUS_OK)
			return turn_number;
	}
	throwb(ex_pos, is_modbus_error);
	return turn_number;
}
// 旧代码（无重试）:
// static void Get_Turn_Cb_old(ModBusFailCode_Type err,u8* data,u16 len)
// {
// 	wait_flag = 0;
// 	is_modbus_error = err;
// 	turn_number = data[5]<<24 | data[6]<<16 | data[3]<<8 | data[4];
// }
// s32 Get_Turn_Number_old(void)
// {
// 	wait_flag = 1;
// 	is_modbus_error = 0;
// 	Send_Func03_Data(TURN_NUMBER_REGISTER, 2, Get_Turn_Cb_old);
// 	while(wait_flag);
// 	if(is_modbus_error != 0)
// 		throwb(ex_pos,is_modbus_error);
// 	return turn_number;
// }

void Check_Fault(void)
{
	Send_Func03_Data(FAULT_REGISTER, 1, 0);
}

static void Debug_EchoEnc_Cb(ModBusFailCode_Type err, u8 *data, u16 len)
{
	wait_flag = 0;
	is_modbus_error = err;
	encoder_number = data[5]<<24 | data[6]<<16 | data[3]<<8 | data[4];
	printf("ENC[len=%d]:", len);
	for (u16 i = 0; i < len; i++)
		printf(" %02X", data[i]);
	printf("\r\n");
}

static void Debug_EchoTurn_Cb(ModBusFailCode_Type err, u8 *data, u16 len)
{
	wait_flag = 0;
	is_modbus_error = err;
	turn_number = data[5]<<24 | data[6]<<16 | data[3]<<8 | data[4];
	printf("TRN[len=%d]:", len);
	for (u16 i = 0; i < len; i++)
		printf(" %02X", data[i]);
	printf("\r\n");
}

void Debug_ReadMotorMulti(void)
{
	printf("\r\n=== Motor Debug: 10 enc + 10 turn ===\r\n");
	for (u8 i = 0; i < 10; i++)
	{
		wait_flag = 1;
		is_modbus_error = 0;
		Send_Func03_Data(ENCODER_NUMBER_REGISTER, 2, Debug_EchoEnc_Cb);
		while (wait_flag);

		wait_flag = 1;
		is_modbus_error = 0;
		Send_Func03_Data(TURN_NUMBER_REGISTER, 2, Debug_EchoTurn_Cb);
		while (wait_flag);
	}
	printf("=== Done ===\r\n");
}




