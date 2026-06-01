#include "bll_eeprom.h" 
#include "modbus_save_data.h"
//#include "eeprom_driver.h"

static CommonStateFlag_Type flag = CSF_Idel;

static void Save_Data(void)
{
	FUN_EEPROM_WRITE((u8 *)SYSTEM_REG, EEPROM_BASE_ADDR, sizeof(REGION_TYPE(0)));
	FUN_EEPROM_WRITE((u8 *)RUN_REG, EEPROM_BASE_ADDR, sizeof(REGION_TYPE(1)));
}

static void Read_Default(void)
{
	FUN_EEPROM_READ((u8 *)SYSTEM_REG, EEPROM_DEFAULT_ADDR, sizeof(REGION_TYPE(0)));
	FUN_EEPROM_READ((u8 *)RUN_REG, EEPROM_DEFAULT_ADDR, sizeof(REGION_TYPE(1)));
}

CommonStateFlag_Type BLL_EEPROM_Execute(ParamShadow_Type params ,u8 *err)
{
	if (flag == CSF_Idel)
	{
		flag = CSF_Working;
		if (params.Param1 == 1)
			Save_Data();
		else if (params.Param1 == 2)
			Read_Default();
	}
	//if(finished)
	//flag = CSF_Finished;
	*err = 0;
	return flag;
}

void BLL_EEPROM_ClearFlag(void)
{
	flag = CSF_Idel;
}
