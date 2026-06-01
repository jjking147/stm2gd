#ifndef __MOTOR_H
#define __MOTOR_H
#include "sys.h"

#define DI1_FUNC_REGISTER                  0x2504
#define DI2_FUNC_REGISTER                  0x2505
#define DI3_FUNC_REGISTER                  0x2506
#define DI4_FUNC_REGISTER                  0x2507

#define FAULT_REGISTER                     0x603f
#define CONTROL_REGISTER                   0x6040
#define STATUS_REGISTER                    0x6041
#define STOP_REGISTER                      0x605a
#define MODOL_REGISTER                     0x6060
#define POSITION_MODE_SPEED_REGISTER       0x6081
#define SPEED_MODE_SPEED_REGISTER          0x60ff
#define ACCE_REGISTER                      0x6083
#define DECE_REGISTER                      0x6084
#define DISTANCE_REGISTER                  0x607a
#define ERROR_REGISTER                     0x603f
#define DECE_REGISTER                      0x6084
#define BACK_ZERO_MODE_REGISTER            0x6098
#define BACK_ZERO_SPEED_REGISTER           0x6099
#define BACK_ZERO_ACCE_REGISTER            0x609A

#define DRIVER_POWER_ON_CMD                0x0006
#define INIT_PARA_CMD                      0x0007
#define STOP_CMD                           0x000b
#define MOROR_ENABLE_CMD                   0x000f
#define SET_ABSO_POSITION_CMD              0x000f
#define SET_RELA_POSITION_CMD              0x004f
#define ABSO_POSITION_START_SAMPLE_CMD     0x001f
#define RELA_POSITION_START_SAMPLE_CMD     0x005f
#define FREE_STOP_CMD                      0x0000
#define DECE_STOP_MOTOR_DISABLE_CMD        0x0001
#define FAST_DECE_STOP_MOTOR_DISABLE_CMD   0x0002
#define DECE_STOP_MOTOR_ENABLE_CMD         0x0005
#define FAST_DECE_STOP_MOTOR_ENABLE_CMD    0x0006
#define SET_NEG_LIMIT_CMD                  0x0011
#define SET_POS_LIMIT_CMD                  0x0012

#define POSITION_MODOL_CMD                 0x0001
#define CHECK_STATUS_CMD                   0x0001
#define SPEED_MODOL_CMD                    0x0003
#define BACK_ZERO_MODOL_CMD                0x0006

void Motor_Init(void);
void Driver_Power_On(void);
void Init_Parameter(void);
void Motor_Enable(void);
void Stop(void);
void Free_Stop(void);
void Fast_Dece_Stop_Motor_Disable(void);
void Dece_Stop_Motor_Disable(void);
void Fast_Dece_Stop_Motor_Enable(void);
void Dece_Stop_Motor_Enable(void);
void Set_Position_Mode(void);
void Set_Speed_Mode(void);
void Position_Mode_Set_Speed(s32 Speed);
void Speed_Mode_Set_Speed(s32 Speed1);
void Set_Acce(u16 Acce);
void Set_Dece(u16 Dece);
void Set_Distence(u16 Distence);
void Set_Abso_Position_Mode(void);
void Set_Rela_Position_Mode(void);
void Set_Back_Zero_Mode(void);
void Abso_Position_Start_Sample(void);
void Rela_Position_Start_Sample(void);
void Set_Neg_Limit_Mode(void);
void Set_Back_Zero_Acce(u16 Acce);
void Set_Back_Zero_Speed(u16 Speed);
void Set_DI1_Func(u16 Func);
#endif

