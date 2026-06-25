"""
PC直接读电机寄存器（模拟GD32主机）
用于测试电机返回的数据是否带0xFF噪声
用法: python pc_read_motor.py COMx
"""
import sys
import serial
import struct
import time

def crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc

def build_read_cmd(addr, reg, count):
    cmd = struct.pack('>BBHH', addr, 0x03, reg, count)
    crc = crc16(cmd)
    return cmd + struct.pack('<H', crc)  # CRC LO-HI

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM6"
    motor_addr = 0x03

    ser = serial.Serial(port, 115200, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE, timeout=0.5)
    print(f"opened {port}")
    print(f"motor addr: 0x{motor_addr:02X}\n")

    for i in range(20):
        # 读编码器 0x0066 (2寄存器)
        cmd = build_read_cmd(motor_addr, 0x0066, 2)
        cmd_hex = ' '.join(f'{b:02X}' for b in cmd)
        ser.write(cmd)
        time.sleep(0.1)

        resp = ser.read(ser.in_waiting)
        if resp:
            resp_hex = ' '.join(f'{b:02X}' for b in resp)
            print(f"ENC[{i+1:2d}] TX: {cmd_hex}")
            print(f"     RX(len={len(resp):2d}): {resp_hex}")
        else:
            print(f"ENC[{i+1:2d}] TX: {cmd_hex}")
            print(f"     RX: (timeout)")

        time.sleep(0.2)

        # 读圈数 0x0068 (2寄存器)
        cmd = build_read_cmd(motor_addr, 0x0068, 2)
        cmd_hex = ' '.join(f'{b:02X}' for b in cmd)
        ser.write(cmd)
        time.sleep(0.1)

        resp = ser.read(ser.in_waiting)
        if resp:
            resp_hex = ' '.join(f'{b:02X}' for b in resp)
            print(f"TRN[{i+1:2d}] TX: {cmd_hex}")
            print(f"     RX(len={len(resp):2d}): {resp_hex}")
        else:
            print(f"TRN[{i+1:2d}] TX: {cmd_hex}")
            print(f"     RX: (timeout)")

        print()

    ser.close()

if __name__ == '__main__':
    main()
