#include "modbus_save_data.h"
#include "config.h"

REGION_TYPE(0) Reg_4x_0 = {
	0,          // Reserved1
	VERSION,    // Version
	DRIVERTYPE, // DrvType
	1152,       // BaudRate
	0,          // IsSkipCRC
	0,          // IODebounceTime
	0,          // CounterFilter
	0,          // EStopType
	0,          // Motor1Addr
	0,          // Motor2Addr
	0           // ClawAddr
};

REGION_TYPE(1) Reg_4x_1 = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

REGION_TYPE(2) Reg_4x_2 = {
	{0, 0},     // Position
	0,          // PlaceFilled
	0,          // StretchState
	0,          // ClawState
	0,          // RollerState
	0,          // WorkingProcess
	0           // LastMotorState
};

REGION_TYPE(3) Reg_4x_3 = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

REGION_TYPE(4) Reg_4x_4 = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	{0, 0, 0, 0, 0}, // State
	0                  // FailureCode
};

REGION_TYPE(5) Reg_4x_5 = {
	0, 0, 0, 0, 0, 0, 0, 0
};

u16 Reg_4x_6[100];

void* Reg_Regions[] =
{
	&Reg_4x_0,
	&Reg_4x_1,
	&Reg_4x_2,
	&Reg_4x_3,
	&Reg_4x_4,
	&Reg_4x_5,
	&Reg_4x_6,
};

static u8 _RegionSize[] =
{
	sizeof(REGION_TYPE(0)),
	sizeof(REGION_TYPE(1)),
	sizeof(REGION_TYPE(2)),
	sizeof(REGION_TYPE(3)),
	sizeof(REGION_TYPE(4)),
	sizeof(REGION_TYPE(5)),
	sizeof(Reg_4x_6),
};

u8 ValidateRegRange(u8 region, u8 subaddr)
{
	if (region >= sizeof(Reg_Regions))
		return 0;
	if (subaddr >= _RegionSize[region])
		return 0;

	return 1;
}
