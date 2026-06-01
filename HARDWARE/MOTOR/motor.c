#include "sys.h"
#include "motor.h"
#include "usart.h" 
#include "modbus_master.h"

extern u8 MOTOR_ADDRESS;

void Motor_Init(void)
{
	Driver_Power_On();
	Init_Parameter();
	Motor_Enable();
}

void Driver_Power_On(void)//�������
{
	Send_Func06_Data(CONTROL_REGISTER, DRIVER_POWER_ON_CMD);
}

void Init_Parameter(void)
{
	Send_Func06_Data(CONTROL_REGISTER, INIT_PARA_CMD);
}

void Motor_Enable(void)
{
	Send_Func06_Data(CONTROL_REGISTER, MOROR_ENABLE_CMD);
}

void Stop(void)
{
	Send_Func06_Data(CONTROL_REGISTER, STOP_CMD);
}


//����ֹͣ����
void Free_Stop(void)
{
	Send_Func06_Data(STOP_REGISTER, FREE_STOP_CMD);//0000
}

void Dece_Stop_Motor_Disable(void)
{
	Send_Func06_Data(STOP_REGISTER, DECE_STOP_MOTOR_DISABLE_CMD);//0001
}

void Fast_Dece_Stop_Motor_Disable(void)//0002
{
	Send_Func06_Data(STOP_REGISTER, FAST_DECE_STOP_MOTOR_DISABLE_CMD);
}

void Dece_Stop_Motor_Enable(void)//0005
{
	Send_Func06_Data(STOP_REGISTER, DECE_STOP_MOTOR_ENABLE_CMD);
}

void Fast_Dece_Stop_Motor_Enable(void)//0006
{
	Send_Func06_Data(STOP_REGISTER, FAST_DECE_STOP_MOTOR_ENABLE_CMD);
}

//�˶�ģʽ���ƼĴ���
void Set_Position_Mode(void)
{
	u8 err = Send_Func06_Data(MODOL_REGISTER, POSITION_MODOL_CMD);
}

void Set_Speed_Mode(void)
{
	Send_Func06_Data(MODOL_REGISTER, SPEED_MODOL_CMD);
}

void Speed_Mode_Set_Speed(s32 Speed1)
{
	Send_Func10_Data(SPEED_MODE_SPEED_REGISTER, (s16)Speed1);
}

void Set_Back_Zero_Mode(void)
{
	Send_Func06_Data(MODOL_REGISTER, BACK_ZERO_MODOL_CMD);
}

void Set_Acce(u16 Acce)
{
	Acce = Acce * 10;
	Send_Func10_Data(ACCE_REGISTER, Acce);//���ٶ�
}

void Set_Dece(u16 Dece)
{
	Dece = Dece * 10;
	Send_Func10_Data(DECE_REGISTER, Dece);//���ٶ�
}

void Set_Distence(u16 Distence)
{
	Send_Func10_Data(DISTANCE_REGISTER, Distence);
}

void Set_Abso_Position_Mode(void)//���þ���λ������
{
	Send_Func06_Data(CONTROL_REGISTER, SET_ABSO_POSITION_CMD);
}

void Set_Rela_Position_Mode(void)//�����������λ��
{
	Send_Func06_Data(CONTROL_REGISTER, SET_RELA_POSITION_CMD);
}

void Abso_Position_Start_Sample(void)//��ʼ����λ�ò���
{
	Send_Func06_Data(CONTROL_REGISTER, ABSO_POSITION_START_SAMPLE_CMD);
}

void Rela_Position_Start_Sample(void)//��ʼ���λ�ò�����
{
	Send_Func06_Data(CONTROL_REGISTER, RELA_POSITION_START_SAMPLE_CMD);
}

void Set_Neg_Limit_Mode(void)
{
	Send_Func06_Data(BACK_ZERO_MODE_REGISTER, SET_NEG_LIMIT_CMD);
}

void Set_Back_Zero_Acce(u16 Acce)
{
	Acce = Acce * 10;
	Send_Func10_Data(BACK_ZERO_ACCE_REGISTER, Acce);
}

void Set_Back_Zero_Speed(u16 Speed)
{
	Send_Func10_Data(BACK_ZERO_SPEED_REGISTER, Speed);
}

void Set_DI1_Func(u16 Func)
{
	Send_Func06_Data(DI1_FUNC_REGISTER, Func);
}
