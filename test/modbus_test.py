"""
GD32 Modbus RTU 从机通信测试脚本
用法: python modbus_test.py COMx
需要: pip install pyserial
"""
import sys
import serial
import struct
import time

# Modbus CRC16 计算
def crc16(data):
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc

def build_read_cmd(slave_id, func, start_addr, count):
    """构建读寄存器命令"""
    cmd = struct.pack('>BBHH', slave_id, func, start_addr, count)
    crc = crc16(cmd)
    cmd += struct.pack('<H', crc)
    return cmd

def build_write_cmd(slave_id, addr, value):
    """构建写单个寄存器命令 (功能码0x06)"""
    cmd = struct.pack('>BBHH', slave_id, 0x06, addr, value)
    crc = crc16(cmd)
    cmd += struct.pack('<H', crc)
    return cmd

def send_and_receive(ser, cmd, desc, timeout=0.5):
    """发送命令并接收响应"""
    print(f"\n>>> {desc}")
    print(f"    发送: {cmd.hex(' ').upper()}")
    ser.write(cmd)
    time.sleep(timeout)
    if ser.in_waiting:
        resp = ser.read(ser.in_waiting)
        print(f"    收到: {resp.hex(' ').upper()} ({len(resp)} 字节)")
        # 验证CRC
        if len(resp) >= 4:
            data_part = resp[:-2]
            recv_crc = struct.unpack('<H', resp[-2:])[0]
            calc_crc = crc16(data_part)
            if recv_crc == calc_crc:
                print(f"    CRC校验: 通过")
            else:
                print(f"    CRC校验: 失败 (期望{calc_crc:04X}, 收到{recv_crc:04X})")
        # 解析响应
        if len(resp) >= 5 and resp[1] == 0x03:
            byte_count = resp[2]
            values = []
            for i in range(byte_count // 2):
                val = struct.unpack('>H', resp[3 + i*2 : 5 + i*2])[0]
                values.append(val)
            print(f"    寄存器值: {values}")
        elif len(resp) >= 4 and resp[1] == 0x06:
            addr = struct.unpack('>H', resp[2:4])[0]
            val = struct.unpack('>H', resp[4:6])[0]
            print(f"    写入确认: 地址0x{addr:04X} = {val}")
        elif len(resp) >= 3 and resp[1] & 0x80:
            print(f"    !!! 异常响应: 功能码0x{resp[1]:02X}, 异常码0x{resp[2]:02X}")
        return resp
    else:
        print(f"    !!! 无响应 (超时{timeout}s)")
        return None

def main():
    if len(sys.argv) < 2:
        print("用法: python modbus_test.py COMx")
        print("例如: python modbus_test.py COM3")
        sys.exit(1)

    port = sys.argv[1]
    slave_id = 7  # config.h 中 MODBUS_SLAVE_ID

    try:
        ser = serial.Serial(port, 115200, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE, timeout=0.5)
        print(f"已打开 {port}, 115200 8N1")
        print(f"目标从机地址: {slave_id}")
    except Exception as e:
        print(f"打开串口失败: {e}")
        sys.exit(1)

    # 测试1: 读系统寄存器 (Region 0, 地址0x0000, 读5个)
    cmd = build_read_cmd(slave_id, 0x03, 0x0000, 5)
    resp = send_and_receive(ser, cmd, "测试1: 读系统寄存器 (0x0000~0x0004)")

    # 测试2: 读运行状态寄存器 (Region 1, 地址0x0064, 读5个)
    cmd = build_read_cmd(slave_id, 0x03, 0x0064, 5)
    resp = send_and_receive(ser, cmd, "测试2: 读运行状态寄存器 (0x0064~0x0068)")

    # 测试3: 读实时数据寄存器 (Region 2, 地址0x00C8, 读5个)
    cmd = build_read_cmd(slave_id, 0x03, 0x00C8, 5)
    resp = send_and_receive(ser, cmd, "测试3: 读实时数据寄存器 (0x00C8~0x00CC)")

    # 测试4: 读触发命令寄存器 (Region 5, 地址0x01F4, 读5个)
    cmd = build_read_cmd(slave_id, 0x03, 0x01F4, 5)
    resp = send_and_receive(ser, cmd, "测试4: 读触发命令寄存器 (0x01F4~0x01F8)")

    # 测试5: 读任务参数寄存器 (Region 4, 地址0x0190, 读5个)
    cmd = build_read_cmd(slave_id, 0x03, 0x0190, 5)
    resp = send_and_receive(ser, cmd, "测试5: 读任务参数寄存器 (0x0190~0x0194)")

    # 测试6: 写单个寄存器测试 (写入触发寄存器区域的某个安全地址)
    # 先读再写, 测试功能码0x06
    cmd = build_write_cmd(slave_id, 0x01F4, 0x0000)
    resp = send_and_receive(ser, cmd, "测试6: 写单个寄存器 (0x01F4 = 0x0000)")

    # 测试7: 地址不匹配测试 (用错误地址, 应无响应或异常)
    cmd = build_read_cmd(0x01, 0x03, 0x0000, 1)  # 地址=0x01, 不匹配
    resp = send_and_receive(ser, cmd, "测试7: 错误从机地址 (应无响应)", timeout=0.2)

    # 测试8: 非法功能码测试
    cmd = bytes([slave_id, 0x04, 0x00, 0x00, 0x00, 0x01])
    cmd += struct.pack('<H', crc16(cmd))
    resp = send_and_receive(ser, cmd, "测试8: 非法功能码0x04 (应返回异常码)")

    ser.close()
    print("\n" + "="*50)
    print("测试完成!")
    print("如果测试1~6有正常响应, 说明GD32 Modbus从机通信正常。")
    print("测试7无响应说明地址过滤正确。")
    print("测试8返回异常码说明协议解析正确。")

if __name__ == '__main__':
    main()
