#ifndef _TIMER_H
#define _TIMER_H
#include "sys.h"

// GD32 Timer mapping:
// STM32 TIM6 -> GD32 TIMER5
// STM32 TIM7 -> GD32 TIMER6
// STM32 TIM5 -> GD32 TIMER4
// STM32 TIM9 -> GD32 TIMER9

#define MODBUS_SLAVE_TIM TIMER5
#define MODBUS_SLAVE_TIM_IRQn TIMER5_DAC_IRQn

#define MODBUS_MASTER_TIM TIMER6
#define MODBUS_MASTER_TIM_IRQn TIMER6_IRQn

// TIMER4用于延时计数
#define DELAY_TIM TIMER4

#define DISPATCHER_TIM TIMER9
#define DISPATCHER_TIM_IRQn TIMER0_UP_TIMER9_IRQn

#define MODBUS_SLAVE_TIM_CLK_EN rcu_periph_clock_enable(RCU_TIMER5)
#define MODBUS_MASTER_TIM_CLK_EN rcu_periph_clock_enable(RCU_TIMER6)
#define DELAY_TIM_CLK_EN rcu_periph_clock_enable(RCU_TIMER4)
#define DISPATCHER_TIM_CLK_EN() rcu_periph_clock_enable(RCU_TIMER9)

// TIMER4就是DELAY_TIM,用于延时计数
#define ReadTick() (TIMER_CNT(DELAY_TIM) / 2)
#define TickSpan(start) ((TIMER_CNT(DELAY_TIM) / 2) - start)

void Modbus_Slave_Timer_Init(int ms);
void Modbus_Master_Timer_Init(int ms);
void Delay_Timer_Init(void);
void Dispatcher_Tim_Init(u16 ms);

#endif
