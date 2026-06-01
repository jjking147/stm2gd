#ifndef __BLL_ACCESS_H__ 
#define __BLL_ACCESS_H__ 
 
 
#include "sys.h"
#include "bll_adapter.h"
#include "config.h"
#include "bll_main.h"

void BLL_Access_ClearFlag(void);
CommonStateFlag_Type BLL_Access_Execute(ParamShadow_Type params, u8 *err);
 
#endif 
 
