#pragma once

#include "common.h"
#include "modbus_save_data.h"

typedef struct ParamShadow_Type
{
	MissionID_Type MissionID;
	u16 Param1;
	u16 Param2;
	u16 Param3;
	u16 Param4;
	u16 Param5;
	u16 Param6;
	u16 Param7;
	u16 Param8;
	u16 Param9;
	u16 Param10;
}ParamShadow_Type;

#define System_State PARAM_REG.State.SystemState
#define System_Inited PARAM_REG.State.IsInited
#define System_Failure PARAM_REG.FailureCode

extern Trigger_Type Current_Trigger;


void BLL_Init_All(void);
void BLL_Execute_Mission(void);
