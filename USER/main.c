#include "gd32f4xx.h"
#include "delay.h"
#include "usart.h"
#include "bll_main.h"
#include "modbus_slave.h"
#include "timer.h"
#include "jexception.h"

#include "gpio.h"

INIT_EXCEPTION_SYSTEM;

u8 MOTOR_ADDRESS = 0x03;

u8 IsJumpIAP = 0;
extern void BLL_Update(void);

int main(void)
{
	SystemInit();
	BLL_Init_All();
	delay_init(168);

	while (1)
	{
		BLL_Execute_Mission();
		if(IsJumpIAP)
			BLL_Update();
	}
}
