#pragma once
#include "sys.h"

#define DEF_REGION_TYPE(n) typedef struct Reg_4x_Region##n##_Type
#define REGION_TYPE(n) Reg_4x_Region##n##_Type
#define GET_REG_REGION(n) (Reg_4x_##n)
#define GET_REG_REGION_PTR(n) (&Reg_4x_##n)

typedef enum System_State_Type
{
	SYS_Idel,
	SYS_Working,
	SYS_Finished,
	SYS_Faliure
} System_State_Type;

typedef enum EStop_type
{
	ESTOP_Bus,
	ESTOP_IO,
	ESTOP_Brake,
	ESTOP_Relay
} EStop_type;

typedef enum PositionOffset_Type
{
	POSO_Unkonw,
	POSO_OK,
	POSO_Untouched,
	POSO_Exceed
} PositionOffset_Type;

typedef struct Position_Type
{
	PositionOffset_Type Offset;
	u8 Position;
} Position_Type;

typedef enum MissionID_Type
{
	Misson_None,
	Misson_EEROM,
	Misson_Zero,
	Misson_Move,
	Mission_MoveRaw,
	Mission_Aim,
	Mission_Roller,
	Mission_Claw,
	Mission_ForwardData
} MissionID_Type;

typedef enum FailureID_Type
{
	Failure_None,
	Failure_Actuator,
	Failure_Timeout,
	Failure_Counter,
	Failure_Aim,
	Failure_Zero,
	Failure_Unsupported,
	Failure_Argument,
	Failure_OverBound,
	Failure_EStop = 0x10,
	Failure_Limit = 0x20,
	Failure_Unknown = 0xF0
} FailureID_Type;

typedef enum Trigger_Type
{
	TRIG_MissionStart,
	TRIG_ClearFinishFlag,
	TRIG_PauseMission,
	TRIG_ResumeMission,
	TRIG_StopMission,
	TRIG_AbortMission,
	TRIG_ClearFailure,
	TRIG_SoftReset,
	TRIG_None = 0xFF,
} Trigger_Type;

DEF_REGION_TYPE(0)
{
	u16 Reserved1;
	u8 Version;
	u8 DrvType;
	u16 BaudRate;
	u16 IsSkipCRC;
	u16 IODebounceTime;
	u16 CounterFilter;
	EStop_type EStopType;
	u16 Motor1Addr;
	u16 Motor2Addr;
	u16 ClawAddr;
}
REGION_TYPE(0);

DEF_REGION_TYPE(1)
{
	u16 Locked;
	u16 MaxWorkTimeout;
	u32 MaxSpeed;
	u32 MaxAcc;
	u32 MaxDec;
	u32 FastStopSpeed;
	u32 ZeroSpeed;
	u32 ZeroAcc;
	u32 SlowSpeed;
	u32 NormalSpeed;
}
REGION_TYPE(1);

DEF_REGION_TYPE(2)
{
	Position_Type Position;
	u16 PlaceFilled;
	u16 StretchState;
	u16 ClawState;
	u16 RollerState;
	u16 WorkingProcess;
	u16 LastMotorState;
}
REGION_TYPE(2);

DEF_REGION_TYPE(3)
{
	u16 IO1State;
	u16 IO2State;
	u16 IO3State;
	u16 IO4State;
	u16 IO5State;
	u16 IO6State;
	u16 IO7State;
	u16 IO8State;
	u16 IO9State;
	u16 IO10State;
	u16 PhotoCounter;
}
REGION_TYPE(3);

DEF_REGION_TYPE(4)
{
	MissionID_Type MissionID;
	u16 Paream1;
	u16 Paream2;
	u16 Paream3;
	u16 Paream4;
	u16 Paream5;
	u16 Paream6;
	u16 Paream7;
	u16 Paream8;
	u16 Paream9;
	u16 Paream10;
	__packed struct
	{
		System_State_Type SystemState : 2;
		u8 Reserved : 2;
		u8 IsInited : 1;
		u8 Reserved2 : 3;
		u8 MissionStep;
	} State;
	FailureID_Type FailureCode;
}
REGION_TYPE(4);

DEF_REGION_TYPE(5)
{

	u16 MissionStart;
	u16 ClearFinishFlag;
	u16 PauseMission;
	u16 ResumeMission;
	u16 StopMission;
	u16 AbortMission;
	u16 ClearFailureFlag;
	u16 SoftReset;
}
REGION_TYPE(5);

extern REGION_TYPE(0) Reg_4x_0;
extern REGION_TYPE(1) Reg_4x_1;
extern REGION_TYPE(2) Reg_4x_2;
extern REGION_TYPE(3) Reg_4x_3;
extern REGION_TYPE(4) Reg_4x_4;
extern REGION_TYPE(5) Reg_4x_5;
extern u16 Reg_4x_6[];
extern void *Reg_Regions[];

#define SYSTEM_REG Reg_4x_0
#define RUN_REG Reg_4x_1
#define REALTIME_REG Reg_4x_2
#define RAW_REG Reg_4x_3
#define PARAM_REG Reg_4x_4
#define EXECUTE_REG Reg_4x_5

u8 ValidateRegRange(u8 region, u8 subaddr);
