# GD32F4 LiftV1.6 迁移说明

## 文件状态

本项目已完成STM32F4到GD32F4的代码迁移，所有官方固件库文件已从 GD32F4xx_Firmware_Library_V3.2.0 复制完成。

### 已包含的官方文件

#### CORE目录 - CMSIS核心文件
- core_cm4.h
- core_cm4_simd.h
- core_cmFunc.h
- core_cmInstr.h
- startup_gd32f407.s (启动文件)

#### USER目录 - 系统文件
- gd32f4xx.h (官方主头文件)
- system_gd32f4xx.h/c (官方系统时钟配置)
- gd32f4xx_it.h/c (官方中断处理模板)
- gd32f4xx_libopt.h (外设库配置文件)

#### FWLIB目录 - 标准外设库
- inc/ - 所有外设头文件
- src/ - 所有外设源文件

### 重要说明

官方固件库使用 `gd32f4xx_libopt.h` 而不是 `gd32f4xx_conf.h` 作为外设配置文件。

### 主要迁移修改点

1. **头文件引用**: `stm32f4xx.h` -> `gd32f4xx.h`
2. **系统配置**: `system_stm32f4xx.c` -> `system_gd32f4xx.c`
3. **中断处理**: `stm32f4xx_it.c` -> `gd32f4xx_it.c`
4. **外设配置**: `stm32f4xx_conf.h` -> `gd32f4xx_conf.h`
5. **启动文件**: `startup_stm32f407xx.s` -> `startup_gd32f407.s`

### API映射关系

| STM32 | GD32 |
|-------|------|
| RCC_AHB1PeriphClockCmd() | rcu_periph_clock_enable() |
| GPIO_Init() | gpio_init() |
| GPIO_SetBits() | gpio_bit_set() |
| GPIO_ResetBits() | gpio_bit_reset() |
| USART_Init() | usart_init() |
| USART_Cmd() | usart_enable() |
| USART_SendData() | usart_data_transmit() |
| USART_ReceiveData() | usart_data_receive() |
| TIM_TimeBaseInit() | timer_init() |
| TIM_Cmd() | timer_enable() / timer_disable() |
| EXTI_Init() | exti_init() |
| IWDG_Enable() | fwdgt_enable() |
| FLASH_Unlock() | fmc_unlock() |
| FLASH_ProgramWord() | fmc_word_program() |
| NVIC_Init() | nvic_irq_enable() |

### 注意事项

1. GD32F407通常使用8MHz HSE晶振（STM32F407使用25MHz）
2. 时钟配置需要根据实际硬件调整
3. Flash等待周期可能不同
4. 某些外设的寄存器地址可能有细微差异

### Keil工程配置

1. Device: 选择 GD32F407VET6 或对应型号
2. 预定义宏: 添加 `GD32F40X` 或 `GD32F40_41xxx`
3. Include Paths: 添加GD32库的头文件路径
