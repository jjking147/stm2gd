#include "exti.h"
#include "delay.h"
#include "gpio.h"
#include "bll_motor.h"
#include "bll_tocase.h"

void NVIC_Config(u8 num, u8 edge)
{
    exti_trig_type_enum trig_type = EXTI_TRIG_RISING;

    if (edge == 0)
    {
        trig_type = EXTI_TRIG_RISING;
    }
    else
    {
        trig_type = EXTI_TRIG_FALLING;
    }

    rcu_periph_clock_enable(RCU_SYSCFG);

    switch (num)
    {
    case 1:
        syscfg_exti_line_config(EXTI_SOURCE_GPIOC, EXTI_SOURCE_PIN2);
        exti_init(EXTI_2, EXTI_INTERRUPT, trig_type);
        nvic_irq_enable(EXTI2_IRQn, 0, 2);
        break;

    case 2:
        syscfg_exti_line_config(EXTI_SOURCE_GPIOC, EXTI_SOURCE_PIN3);
        exti_init(EXTI_3, EXTI_INTERRUPT, trig_type);
        nvic_irq_enable(EXTI3_IRQn, 0, 2);
        break;

    case 3:
        syscfg_exti_line_config(EXTI_SOURCE_GPIOC, EXTI_SOURCE_PIN4);
        exti_init(EXTI_4, EXTI_INTERRUPT, trig_type);
        nvic_irq_enable(EXTI4_IRQn, 0, 2);
        break;

    case 4:
        syscfg_exti_line_config(EXTI_SOURCE_GPIOC, EXTI_SOURCE_PIN5);
        exti_init(EXTI_5, EXTI_INTERRUPT, trig_type);
        nvic_irq_enable(EXTI5_9_IRQn, 0, 2);
        break;

    case 5:
        syscfg_exti_line_config(EXTI_SOURCE_GPIOC, EXTI_SOURCE_PIN7);
        exti_init(EXTI_7, EXTI_INTERRUPT, trig_type);
        nvic_irq_enable(EXTI5_9_IRQn, 0, 2);
        break;

    case 6:
        syscfg_exti_line_config(EXTI_SOURCE_GPIOC, EXTI_SOURCE_PIN8);
        exti_init(EXTI_8, EXTI_INTERRUPT, trig_type);
        nvic_irq_enable(EXTI5_9_IRQn, 0, 2);
        break;

    case 7:
        syscfg_exti_line_config(EXTI_SOURCE_GPIOC, EXTI_SOURCE_PIN9);
        exti_init(EXTI_9, EXTI_INTERRUPT, trig_type);
        nvic_irq_enable(EXTI5_9_IRQn, 0, 2);
        break;

    default:
        break;
    }
}

static const exti_line_enum exti_lines[] =
{
    EXTI_2,
    EXTI_3,
    EXTI_4,
    EXTI_5,
    EXTI_7,
    EXTI_8,
    EXTI_9
};

void My_EXTI_Cmd(u8 line, u8 cmd)
{
    if (cmd)
        exti_interrupt_enable(exti_lines[line]);
    else
        exti_interrupt_disable(exti_lines[line]);
}

void My_EXTI_DisableAll(void)
{
    u8 len = sizeof(exti_lines) / sizeof(exti_line_enum);
    for (u8 i = 0; i < len; i++)
    {
        exti_interrupt_disable(exti_lines[i]);
    }
}

void EXTI0_IRQHandler(void)
{
    if (exti_flag_get(EXTI_0) != RESET)
    {
        exti_flag_clear(EXTI_0);
    }
}

void EXTI1_IRQHandler(void)
{
    if (exti_flag_get(EXTI_1) != RESET)
    {
        exti_flag_clear(EXTI_1);
    }
}

void EXTI2_IRQHandler(void)
{
    if (exti_flag_get(EXTI_2) != RESET)
    {
        SWTICH1_INT_HANDLER();
        exti_flag_clear(EXTI_2);
    }
}

void EXTI3_IRQHandler(void)
{
    if (exti_flag_get(EXTI_3) != RESET)
    {
        SWTICH2_INT_HANDLER();
        exti_flag_clear(EXTI_3);
    }
}

void EXTI4_IRQHandler(void)
{
    if (exti_flag_get(EXTI_4) != RESET)
    {
        SWTICH3_INT_HANDLER();
        exti_flag_clear(EXTI_4);
    }
}

void EXTI5_9_IRQHandler(void)
{
    if (exti_interrupt_flag_get(EXTI_5) != RESET)
    {
        SWTICH4_INT_HANDLER();
        exti_interrupt_flag_clear(EXTI_5);
    }
    if (exti_interrupt_flag_get(EXTI_6) != RESET)
    {
        GPIO_INT_HANDLER(7);
        exti_interrupt_flag_clear(EXTI_6);
    }
    if (exti_interrupt_flag_get(EXTI_7) != RESET)
    {
        SWTICH5_INT_HANDLER();
        exti_interrupt_flag_clear(EXTI_7);
    }
    if (exti_interrupt_flag_get(EXTI_8) != RESET)
    {
        EStop_INT_HANDLER(1);
        exti_interrupt_flag_clear(EXTI_8);
    }
    if (exti_interrupt_flag_get(EXTI_9) != RESET)
    {
        EStop_INT_HANDLER(0);
        exti_interrupt_flag_clear(EXTI_9);
    }
}
