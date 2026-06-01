#ifndef __EXTI_H__
#define __EXTI_H__
#include "sys.h"

#define GPIO_INT_HANDLER(n)

void NVIC_Config(u8 num,u8 edge);
void My_EXTI_Cmd(u8 line,u8 cmd);
void My_EXTI_DisableAll(void);

#endif
