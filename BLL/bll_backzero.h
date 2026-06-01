#ifndef __BLL_BACKZERO_H__ 
#define __BLL_BACKZERO_H__ 
 
 
#include "sys.h"
#include "bll_adapter.h"
#include "config.h"
#include "bll_main.h"

void BLL_BackZero_ClearFlag(void);
CommonStateFlag_Type BLL_BackZero_Execute(ParamShadow_Type params, u8 *err);

void BackZero(void);

#endif 
 
