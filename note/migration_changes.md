# STM32F407 → GD32F407 移植指南

## 硬件信息
- **MCU**: GD32F407VET6 (与 STM32F407VET6 引脚兼容)
- **晶振**: 8MHz (非 STM32 原版的 25MHz)
- **系统时钟**: 168MHz (PLL: 8MHz / 8 × 336 / 2)
- **调试器**: SWD

---

## 一、移植注意事项 (踩坑总结)

### 1.1 晶振频率必须匹配硬件

GD32 的 `gd32f4xx.h` 默认 HXTAL 为 25MHz。如果实际板载晶振不是 25MHz, 必须修改 `HXTAL_VALUE` 并切换对应的 PLL 配置。否则 PLL 锁定失败, 系统回退到内部 16MHz IRC, 所有时序全部错乱 (波特率、定时器周期等)。

**排查方法**: 如果串口收到的是乱码或无响应, 优先检查晶振配置。

### 1.2 中断处理函数名必须匹配 startup 向量表

STM32 和 GD32 的中断向量表命名不同。移植时**必须逐个核对** startup 文件 (`startup_gd32f407.s`) 中的函数名, 不能直接沿用 STM32 的命名。

**常见陷阱**:
- GD32 头文件中的 IRQ 枚举名可能与 startup 文件中的处理函数名不一致 (如 `TIMER0_UP_TIMER9_IRQn` 实际对应 `TIMER0_UP_TIMER10_IRQHandler`)
- TIMER9/TIMER10/TIMER11 在 GD32F407 上没有独立中断线, 与 TIMER0 共享中断

**排查方法**: 在 `.map` 文件中搜索处理函数名, 确认 startup 向量表引用了你的函数 (而非默认的弱符号)。

### 1.3 DMA 需要选择子外设

GD32F4 的 DMA 通道可以连接到多个外设 (通过 MUX 选择), 这是 STM32F4 没有的机制。使用 DMA 时**必须调用** `dma_channel_subperipheral_select()`, 否则 DMA 通道连接到错误的外设, 传输完成中断不触发。

**排查方法**: DMA 发送后状态机卡死、只响应第一帧命令, 大概率是这个问题。

**参考**: 查阅 GD32F4xx 固件库例程 `Examples/USART/Dma_transmitter&receiver/main.c` 获取正确的 DMA 映射。

### 1.4 USART 标志位不能混用

STM32 StdPeriph 库有些兼容宏会把不同标志位映射到同一个值 (如 `USART_FLAG_TC` → `USART_FLAG_TBE`)。GD32 的 `usart_flag_get()` 原生区分这些标志, 错误映射会导致等待条件不对。

**注意区分**:
- `USART_FLAG_TC` (Transmission Complete): 整个帧 (含 stop bit) 发完
- `USART_FLAG_TBE` (Transmit Buffer Empty): 数据寄存器空, 可写下一字节

### 1.5 定时器时钟需注意 APB 倍频

GD32 的 `rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4)` 设置定时器时钟为 APB 时钟的 4 倍。但 TIMER9 在 APB2 上 (84MHz), 实际定时器时钟为 336MHz, 与 APB1 上的定时器 (168MHz) 不同。计算预分频和周期时需要注意。

### 1.6 CRC 字节序需与上位机一致

本项目使用 `CRC_ORDER=0` (默认值, **HI-LO** 顺序), 与 STM32 原版和上位机软件一致。虽然标准 Modbus RTU 规定 CRC 为 LO-HI 顺序, 但整个系统 (固件 + 上位机) 统一使用 HI-LO 即可正常工作, 不要随意更改。

---

## 二、实际改动清单

### 2.1 晶振频率配置 (8MHz HXTAL)

**文件**: `USER/gd32f4xx.h`
```c
// 原始值 (STM32 默认 25MHz)
#define HXTAL_VALUE    ((uint32_t)25000000)

// 改为
#define HXTAL_VALUE    ((uint32_t)8000000)
```

**文件**: `USER/system_gd32f4xx.c`
```c
// 切换 PLL 配置: 注释掉 25M, 启用 8M
//#define __SYSTEM_CLOCK_168M_PLL_25M_HXTAL       (uint32_t)(168000000)
#define __SYSTEM_CLOCK_168M_PLL_8M_HXTAL        (uint32_t)(168000000)
```

**原因**: 板载晶振为 8MHz, 使用 25MHz 配置会导致 PLL 锁定失败。

---

### 2.2 USART 标志位映射修复

**文件**: `SYSTEM/sys/sys.h`

```c
// 删除这行 (TC 和 TBE 是不同的标志位, GD32 原生有 USART_FLAG_TC)
// #define USART_FLAG_TC    USART_FLAG_TBE

// 保留的映射:
#define USART_FLAG_TXE   USART_FLAG_TBE
#define USART_FLAG_RXNE  USART_FLAG_RBNE
#define USART_IT_RXNE    USART_INT_RBNE
```

**原因**: `USART_FLAG_TC` 和 `USART_FLAG_TBE` 是两个独立标志, GD32 原生支持, 不能混为一谈。

---

### 2.3 中断处理函数名修复

**文件**: `HARDWARE/TIMER/timer.c`

| 原始名 (错误) | 修正名 | 作用 |
|---|---|---|
| `TIMER5_DAC_IRQHandler` | `TIMER5_IRQHandler` | Modbus 帧超时检测 |
| `TIMER0_UP_TIMER9_IRQHandler` | `TIMER0_UP_TIMER10_IRQHandler` | 调度器 (调用 ModBus_Pool + IWDG_Feed) |

**向量表对照** (来自 `startup_gd32f407.s`):
```
Entry 41 (IRQ 25): TIMER0_UP_TIMER10_IRQHandler  ← 调度器用这个
Entry 70 (IRQ 54): TIMER5_IRQHandler              ← Modbus 超时用这个
```

**注意**: GD32F407 的 TIMER9 没有独立中断线, 它的更新事件与 TIMER0 共享 IRQ 25。GD32 头文件中 `TIMER0_UP_TIMER9_IRQn = 25` 的命名具有误导性, 实际对应的处理函数名是 `TIMER0_UP_TIMER10_IRQHandler`。

---

### 2.4 DMA 子外设选择

**文件**: `HARDWARE/USART/usart.c`

```c
// 在 dma_single_data_mode_init() 之后添加:
dma_channel_subperipheral_select(DMA1, DMA_CH7, DMA_SUBPERI4);
```

**原因**: GD32F4 的 DMA 通道需要通过 `dma_channel_subperipheral_select()` 指定连接的外设。缺少此调用会导致 DMA 传输完成中断不触发。

**DMA 映射 (GD32F407 USART0)**:
| 方向 | DMA 控制器 | DMA 通道 | 子外设 |
|------|-----------|---------|--------|
| TX | DMA1 | CH7 | DMA_SUBPERI4 |
| RX | DMA1 | CH2 | DMA_SUBPERI4 |

---

### 2.5 CRC 字节序 (保持原始)

**文件**: `MODBUS/modbus_slave.c`

`CRC_ORDER` 未定义 (默认为 0), 使用 **HI-LO** 顺序。与 STM32 原版和上位机软件一致, **无需修改**。

| CRC_ORDER | 存储顺序 | 说明 |
|---|---|---|
| 0 (本项目) | `[HI, LO]` | 与 STM32 原版和上位机软件一致 |
| 1 (标准 Modbus) | `[LO, HI]` | 标准 Modbus RTU 规定的顺序 |

**注意**: 如果要使用通用 Modbus 调试工具 (如 Modbus Poll), 需确认工具支持自定义 CRC 字节序, 或将固件和上位机统一改为 `CRC_ORDER=1`。

---

## 三、已验证可直接沿用的部分

| 模块 | 说明 |
|------|------|
| GPIO 位带操作 | `sys.h` 中的 `PAin(n)` 等宏, GD32F407 的 GPIO 基地址与 STM32 相同, 位带地址计算正确 |
| GPIO 初始化 | `gpio.h` 中的 `INITX(n)` / `XIN(n)` 宏已适配 GD32 的 `gpio_mode_set()` API |
| EXTI 中断 | `exti.c` 已使用 GD32 的 `syscfg_exti_line_config()` + `exti_init()` |
| USART 配置 | `usart.c` 已使用 GD32 的 `usart_baudrate_set()` 等分步配置函数 |
| Timer 配置 | `timer.c` 已使用 GD32 的 `timer_init()` API, 只需注意 handler 名 |
| IWDG | `iwdg.c` 已使用 GD32 的 `fwdgt_*()` API |
| SysTick 延时 | `delay.c` 直接操作 `SysTick->CTRL` 寄存器, STM32/GD32 通用 |
| setjmp 异常 | `jexception.h` 的 try/throw/catch 在两个平台上行为一致 |
| Modbus 协议栈 | `modbus_master.c` / `modbus_slave.c` 的业务逻辑无需改动 |
| Modbus 寄存器映射 | `modbus_save_data.c/h` 的数据结构和地址映射无需改动 |

---

## 四、调试技巧

### 4.1 串口不通
1. 先用 echo_test 固件验证串口硬件 (TX/RX 接线、波特率)
2. 检查 `HXTAL_VALUE` 是否匹配实际晶振
3. 用 `printf` 在 `main()` 入口输出, 确认固件在运行

### 4.2 Modbus 无响应
1. 在 `ModBus_Pool()` 入口加 `printf` 调试输出, 确认函数被调用
2. 检查 `Check_CRC16()` 返回值, 确认 CRC 是否通过
3. 用 `modbus_raw_test.py` 发送简单命令, 排除测试脚本问题

### 4.3 DMA 发送失败
1. 检查是否调用了 `dma_channel_subperipheral_select()`
2. 在 DMA 完成中断处理函数中加 LED 翻转, 确认中断触发
3. 参考 GD32 官方例程确认 DMA 通道和子外设映射

### 4.4 中断不触发
1. 在 `.map` 文件中确认处理函数被链接到正确地址 (而非 startup 的弱符号默认地址)
2. 对比 startup 文件中的向量表名和代码中的处理函数名
3. 用 LED 或 printf 确认中断处理函数是否被执行
