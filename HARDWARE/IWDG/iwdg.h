#ifndef _IWDG_H
#define _IWDG_H
#include "sys.h"
#include "gd32f4xx_fwdgt.h"

void IWDG_Init(u8 prer,u16 rlr);
void IWDG_Feed(void);
#endif
