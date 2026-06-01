#include "timer.h"
#include "iwdg.h"
#include "modbus_slave.h"
#include "modbus_master.h"

void Modbus_Slave_Timer_Init(int ms)
{
	MODBUS_SLAVE_TIM_CLK_EN;

	timer_deinit(MODBUS_SLAVE_TIM);

	timer_parameter_struct timer_initpara;
	timer_initpara.prescaler = (84000 / 2 - 1);
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = (ms << 1) - 1;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(MODBUS_SLAVE_TIM, &timer_initpara);

	timer_interrupt_flag_clear(MODBUS_SLAVE_TIM, TIMER_INT_FLAG_UP);
	timer_interrupt_enable(MODBUS_SLAVE_TIM, TIMER_INT_UP);

	nvic_irq_enable(MODBUS_SLAVE_TIM_IRQn, 1, 2);

	timer_disable(MODBUS_SLAVE_TIM);
}

void TIMER5_DAC_IRQHandler(void)
{
	if (timer_interrupt_flag_get(MODBUS_SLAVE_TIM, TIMER_INT_FLAG_UP) != RESET)
	{
		ModBus_SetTimeout();
		timer_disable(MODBUS_SLAVE_TIM);
		timer_interrupt_flag_clear(MODBUS_SLAVE_TIM, TIMER_INT_FLAG_UP);
	}
}

void Modbus_Master_Timer_Init(int ms)
{
	MODBUS_MASTER_TIM_CLK_EN;

	timer_parameter_struct timer_initpara;
	timer_initpara.prescaler = (84000 / 2 - 1);
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = (ms << 1) - 1;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(MODBUS_MASTER_TIM, &timer_initpara);

	timer_interrupt_flag_clear(MODBUS_MASTER_TIM, TIMER_INT_FLAG_UP);
	timer_interrupt_enable(MODBUS_MASTER_TIM, TIMER_INT_UP);

	nvic_irq_enable(MODBUS_MASTER_TIM_IRQn, 1, 1);

	timer_disable(MODBUS_MASTER_TIM);
}

void TIMER6_IRQHandler(void)
{
	if (timer_interrupt_flag_get(MODBUS_MASTER_TIM, TIMER_INT_FLAG_UP) == SET)
	{
		Master_Set_Timeout();
		timer_disable(MODBUS_MASTER_TIM);
		timer_interrupt_flag_clear(MODBUS_MASTER_TIM, TIMER_INT_FLAG_UP);
	}
}

void Delay_Timer_Init(void)
{
	DELAY_TIM_CLK_EN;

	timer_parameter_struct timer_initpara;
	timer_initpara.prescaler = (84000 / 2 - 1);
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = 0xFFFFFFFF;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(DELAY_TIM, &timer_initpara);

	timer_interrupt_enable(DELAY_TIM, TIMER_INT_UP);
	timer_enable(DELAY_TIM);
}

void Dispatcher_Tim_Init(u16 ms)
{
	DISPATCHER_TIM_CLK_EN();

	timer_parameter_struct timer_initpara;
	timer_initpara.prescaler = (84000 / 2 - 1);
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.period = ((ms * 2) << 1) - 1;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(DISPATCHER_TIM, &timer_initpara);

	timer_interrupt_flag_clear(DISPATCHER_TIM, TIMER_INT_FLAG_UP);
	timer_interrupt_enable(DISPATCHER_TIM, TIMER_INT_UP);

	nvic_irq_enable(DISPATCHER_TIM_IRQn, 1, 3);

	timer_enable(DISPATCHER_TIM);
}

void TIMER0_UP_TIMER9_IRQHandler(void)
{
	if (timer_interrupt_flag_get(DISPATCHER_TIM, TIMER_INT_FLAG_UP) == SET)
	{
		ModBus_Pool();
		IWDG_Feed();
		timer_interrupt_flag_clear(DISPATCHER_TIM, TIMER_INT_FLAG_UP);
	}
}
