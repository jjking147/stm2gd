"""监听GD32串口输出的电机原始帧数据"""
import sys, serial

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM10"
    ser = serial.Serial(port, 115200, timeout=0.1)
    print(f"listening on {port}...")
    while True:
        line = ser.readline()
        if line:
            print(line.decode('ascii', errors='replace').strip())

if __name__ == '__main__':
    main()
