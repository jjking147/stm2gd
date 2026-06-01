#ifndef __BLL_EEPROM_H__ 
#define __BLL_EEPROM_H__ 
 
 
#include "sys.h"
#include "bll_adapter.h"
#include "config.h"
#include "bll_main.h"

CommonStateFlag_Type BLL_EEPROM_Execute(ParamShadow_Type params, u8 *err);
void BLL_EEPROM_ClearFlag(void);

#endif 
 
