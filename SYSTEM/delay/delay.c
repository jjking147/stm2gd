#include "delay.h"
#include "sys.h"

static u8 fac_us = 0;  //us延时倍乘数
static u16 fac_ms = 0; //ms延时倍乘数

//初始化延迟函数
//SYSCLK:系统时钟频率(MHz)
void delay_init(u8 SYSCLK)
{
    /* GD32 SysTick配置: HCLK/8 */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;  // 关闭SysTick
    SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE_Msk; // 使用AHB/8时钟源
    fac_us = SYSCLK / 8;
    fac_ms = (u16)fac_us * 1000;
}

//延时nus
//nus为要延时的us数.
void delay_us(u32 nus)
{
    u32 temp;
    SysTick->LOAD = nus * fac_us;              // 时间加载
    SysTick->VAL = 0x00;                       // 清空计数器
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;  // 开始倒数
    do
    {
        temp = SysTick->CTRL;
    } while ((temp & 0x01) && !(temp & (1 << 16))); // 等待时间到达
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // 关闭计数器
    SysTick->VAL = 0X00;                       // 清空计数器
}

//延时nms
//nms:0~65535
void delay_xms(u16 nms)
{
    u32 temp;
    SysTick->LOAD = (u32)nms * fac_ms;         // 时间加载(SysTick->LOAD为24bit)
    SysTick->VAL = 0x00;                       // 清空计数器
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;  // 开始倒数
    do
    {
        temp = SysTick->CTRL;
    } while ((temp & 0x01) && !(temp & (1 << 16))); // 等待时间到达
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // 关闭计数器
    SysTick->VAL = 0X00;                       // 清空计数器
}

//延时nms
//nms:0~65535
void delay_ms(u16 nms)
{
    u8 repeat = nms / 540;
    u16 remain = nms % 540;
    while (repeat)
    {
        delay_xms(540);
        repeat--;
    }
    if (remain)
        delay_xms(remain);
}
