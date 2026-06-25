# RS485 通信调试记录

## 问题现象

GD32 通过 RS485 与 SVD750-RS 电机驱动通信时，接收到的数据帧尾部经常混入 0xFF 噪声字节，导致帧长度异常（len=9 的正确帧后面跟着大量 0xFF）。

## 排查过程

### 1. 添加调试函数

在 `bll_motor.c` 中添加 `Debug_ReadMotorMulti()`，上电后连续读 10 次编码器 + 10 次圈数，通过 printf 输出 GD32 收到的原始帧数据。

### 2. 连接电机时的测试结果

连接电机驱动时，帧长度不固定，尾部有大量 0xFF：

```
ENC[len=11]: 03 03 04 25 FE 00 01 72 CF FF FF
TRN[len=14]: 03 03 04 0B EA 00 00 FA 23 FF FF FF FF FF
ENC[len=42]: FF 03 03 04 25 FF 00 01 23 0F FF FF FF ...
```

前 9 字节是正确的 Modbus 响应帧，后面全是 0xFF 噪声。

### 3. 用 PC 模拟电机测试

编写 `test/sim_motor.py`，用 PC 的 USB 转 RS485 适配器模拟电机驱动，连接到 GD32 的 Master_USART RS485 总线（断开电机）。

**测试结果：20/20 全部成功，零噪声。**

```
ENC[len=9]: 03 03 04 00 00 25 F8 C2 E1
TRN[len=9]: 03 03 04 00 00 0B EA 5F 4C
（重复 10 次，全部一致）
```

## 结论

| 测试条件 | 结果 |
|---------|------|
| PC 模拟电机 | 100% 成功，无 0xFF 噪声 |
| 连接真实电机 | 帧尾混入 0xFF 噪声 |

**GD32 的 RS485 电路和 USART 接收逻辑正常。0xFF 噪声来自电机驱动端或电机与 GD32 之间的 RS485 总线。**

可能原因：
1. 电机驱动的 RS485 收发器在空闲时拉低/拉高了总线电平
2. 总线缺少偏置电阻（A 线上拉、B 线下拉）
3. 线缆干扰或终端电阻缺失

## 软件修复措施

在 `modbus_master.c` 中添加了以下保护（当前已注释，待恢复）：

1. **首字节校验**：`data[0] != MOTOR_ADDRESS` 时直接丢弃
2. **CRC 校验**：用 `byte[2]`（字节数字段）计算实际帧长度，只校验有效帧部分，忽略尾部噪声
3. **TIM7 超时重置**：每收到一个字节都重置超时计数器（`TIM_SetCounter` 移到 if 外面）

在 `bll_motor.c` 中：
1. `Get_Encoder_Number` / `Get_Turn_Number` 添加最多 10 次重试
2. 回调中仅在 `MODBUS_OK` 时更新编码器/圈数值

## 文件说明

| 文件 | 说明 |
|------|------|
| `test/sim_motor.py` | PC 模拟电机脚本，监听 GD32 的 Modbus 请求并返回预设响应 |
| `test/debug_motor_read.py` | 串口监听脚本，显示 GD32 的调试输出 |
| `BLL/bll_motor.c` | 包含 `Debug_ReadMotorMulti` 调试函数 |
| `MODBUS/modbus_master.c` | CRC 校验（已注释）、首字节校验（已注释）、TIM 重置修复 |
