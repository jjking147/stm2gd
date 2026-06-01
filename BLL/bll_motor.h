#ifndef __BLL_MOTOR_H
#define __BLL_MOTOR_H
#include "sys.h"

#define FAULT_REGISTER                     0x0064
#define ENCODER_NUMBER_REGISTER            0x0066
#define TURN_NUMBER_REGISTER               0x0068
#define POSITION_TRIGGER_REGISTER          0x0385
#define POSITION_REGISTER                  0x0386
#define PLACED_REGISTER                    0x038b
#define POSITION_MODE_SPEED_REGISTER       0x038c
#define POSITION_MODE_ACCE_REGISTER        0x0392
#define POSITION_MODE_DECE_REGISTER        0x0394

#define POSITION_READY_ENABLE_CMD          0x0000
#define POSITION_ENABLE_CMD                0x0001

void Ready_Brake(void);
void Brake(void);
void Ready_Run(void);
void Run(void);
void Position_Mode_Set_Speed(s32 Speed);
void Position_Mode_Set_Acce(s32 Acce);
void Position_Mode_Set_Dece(s32 Dece);
void Set_Position(s32 Position);
u8 Check_Status(void);
s32 Get_Encoder_Number(void);
s32 Get_Turn_Number(void);
void Check_Fault(void);
void SWTICH5_INT_HANDLER(void);
#endif


