#ifndef __BLL_ADAPTER_H__ 
#define __BLL_ADAPTER_H__

#include "sys.h"
#include "bll_motor.h"
extern vs32 zero_encoder_num,zero_turn_num;

typedef enum CommonStateFlag_Type
{
	CSF_Idel,
	CSF_Working,
	CSF_Finished,
} CommonStateFlag_Type;

#define ADAPTER_PLACEHOLD_VAL			0

#define FUN_MOTOR_CLEAR_RX_FLAG()		__nop()
#define FUN_MOTOR_ENABLE()				__nop()
#define FUN_MOTOR_SET_PSPEED(...)		__nop()
#define FUN_MOTOR_SET_ZSPEED(...)		__nop()
#define FUN_MOTOR_SET_MODE(m)			__nop()
#define FUN_MOTOR_TRIGGER_RPOS()		__nop()
#define FUN_MOTOR_TRIGGER_SPEED(s)		__nop()
#define FUN_EEPROM_WRITE(...)			__nop()
#define FUN_EEPROM_READ(...)			__nop()

int FUN_MOTOR_STOP(void);
int FUN_MOTOR_ABORT(void);

u8 BLL_Moter_AD_BackZero(s32 acce, s32 speed);
u8 BLL_Motor_AD_RelativeMove(s32 distance, s32 acce, s32 dece, s32 speed);
u8 BLL_Motor_AD_AbsoluteMove(s32 distance, s32 acce, s32 dece, s32 speed);
u8 BLL_Motor_AD_SpeedMode(s32 acce, s32 dece, s32 speed, u8 dir);
s32 BLL_Motor_AD_Get_Position(void);

void EStop_INT_HANDLER(u8 updown);
u8 Check_LimitTriggered(void);

__forceinline void BLL_Motor_AD_UpdateZero(void)
{
	zero_encoder_num = Get_Encoder_Number();
	zero_turn_num = Get_Turn_Number();
}

#define _2_POW_17_          (1<<17)
#define CONVERT_POSITION(encoder_num,turn_num) ((int)(1.0 * (((short)(turn_num) * _2_POW_17_) + (encoder_num)) * 1000 / _2_POW_17_))

#define CHECK_LIMIT_STOP(err,lbl)	{\
if(Check_LimitTriggered()) \
		{ \
			Brake(); \
			*err = (Failure_Limit); \
			goto lbl; \
		} }\


#endif 
 
