#include "key.h"
#include "delay.h"

//按键初始化函数
void KEY_Init(void)
{
    /* 使能GPIOA, GPIOE时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOE);

    /* 配置PE2, PE3, PE4为输入，上拉 */
    gpio_mode_set(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);

    /* 配置PA0为输入，下拉 */
    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_0);
}

//按键处理函数
//返回按键值
//mode:0,不支持连续按键;1,支持连续按键;
//0，没有任何按键按下
//1，KEY0按下
//2，KEY1按下
//3，KEY2按下
//4，WKUP按下 WK_UP
//注意此函数有响应优先级,KEY0>KEY1>KEY2>WK_UP!!
u8 KEY_Scan(u8 mode)
{
    static u8 key_up = 1; //按键按松开标志
    if (mode) key_up = 1; //支持连按
    if (key_up && (KEY0 == 0 || KEY1 == 0 || KEY2 == 0 || WK_UP == 1))
    {
        delay_ms(10); //去抖动
        key_up = 0;
        if (KEY0 == 0) return 1;
        else if (KEY1 == 0) return 2;
        else if (KEY2 == 0) return 3;
        else if (WK_UP == 1) return 4;
    }
    else if (KEY0 == 1 && KEY1 == 1 && KEY2 == 1 && WK_UP == 0)
        key_up = 1;
    return 0; // 无按键按下
}
