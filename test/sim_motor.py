"""
PC模拟电机：监听GD32的Modbus请求，返回正确的响应
用于测试GD32的RS485接收是否正常（排除电机硬件问题）
用法: python sim_motor.py COMx
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

MOTOR_ADDR = 0x03

# 模拟的寄存器值
ENC_REG = [0x00, 0x00, 0x25, 0xF8]  # 编码器 0x000025F8
TRN_REG = [0x00, 0x00, 0x0B, 0xEA]  # 圈数   0x00000BEA

def build_response(addr, func, data_bytes):
    """构建Modbus响应帧"""
    resp = bytes([addr, func, len(data_bytes)]) + bytes(data_bytes)
    crc = crc16(resp)
    return resp + struct.pack('<H', crc)  # CRC LO-HI

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM3"
    ser = serial.Serial(port, 115200, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE, timeout=0.1)
    print(f"Motor simulator listening on {port}...")
    print(f"Motor address: {MOTOR_ADDR}")
    print(f"Encoder value: 0x{bytes(ENC_REG).hex()}")
    print(f"Turn value:    0x{bytes(TRN_REG).hex()}")
    print("Waiting for GD32 requests...\n")

    while True:
        # 读取请求（最多20字节，等待超时）
        buf = bytearray()
        while True:
            b = ser.read(1)
            if not b:
                if buf:
                    break
                continue
            buf.extend(b)
            # 等后续字节
            while True:
                b = ser.read(1)
                if not b:
                    break
                buf.extend(b)
            break

        if len(buf) < 4:
            continue

        # 解析请求
        req_addr = buf[0]
        req_func = buf[1]

        hex_str = ' '.join(f'{b:02X}' for b in buf)
        print(f"RX: {hex_str}", end="")

        if req_addr != MOTOR_ADDR:
            print(f" (not for us, addr=0x{req_addr:02X})")
            continue

        if req_func == 0x03 and len(buf) >= 8:
            # 读寄存器请求
            reg_addr = (buf[2] << 8) | buf[3]
            reg_count = (buf[4] << 8) | buf[5]

            print(f" -> Read reg 0x{reg_addr:04X}, count={reg_count}", end="")

            if reg_addr == 0x0066 and reg_count == 2:
                resp = build_response(MOTOR_ADDR, 0x03, ENC_REG)
                ser.write(resp)
                resp_hex = ' '.join(f'{b:02X}' for b in resp)
                print(f" -> TX: {resp_hex}")
            elif reg_addr == 0x0068 and reg_count == 2:
                resp = build_response(MOTOR_ADDR, 0x03, TRN_REG)
                ser.write(resp)
                resp_hex = ' '.join(f'{b:02X}' for b in resp)
                print(f" -> TX: {resp_hex}")
            else:
                # 返回异常响应
                resp = bytes([MOTOR_ADDR, req_func | 0x80, 0x02])
                crc = crc16(resp)
                resp += struct.pack('<H', crc)
                ser.write(resp)
                print(f" -> Exception")
        else:
            print(f" (unsupported func=0x{req_func:02X})")

if __name__ == '__main__':
    main()
