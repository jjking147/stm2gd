#include "bll_tocase.h"
#include "delay.h"
#include "usart.h"
#include "modbus_master.h"
#include "motor.h"
#include "gpio.h"

static CommonStateFlag_Type flag = CSF_Idel;
extern u8 swtich_flag;
extern u8 swtich_count;
extern u8 zero_flag;
extern u8 photoelectric_sign;
extern u8 Master_Receive_Buff[MODBUS_BUFF_LEN];

// 清除flag状态 - 已移至bll_tocase.c
// void BLL_ToCase_ClearFlag(void) { ... }

// BLL_ToCase_Execute - 已移至bll_tocase.c
// CommonStateFlag_Type BLL_ToCase_Execute(ParamShadow_Type params, u8 *err) { ... }
