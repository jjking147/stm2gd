#include "led.h"

//LED IO初始化
void LED_Init(void)
{
    /* 使能GPIOF时钟 */
    rcu_periph_clock_enable(RCU_GPIOF);

    /* 配置PF9, PF10为推挽输出 */
    gpio_mode_set(GPIOF, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_9 | GPIO_PIN_10);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9 | GPIO_PIN_10);

    /* 默认输出高电平，LED灭 */
    gpio_bit_set(GPIOF, GPIO_PIN_9 | GPIO_PIN_10);
}
