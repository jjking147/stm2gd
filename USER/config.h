#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "sys.h"
#include "modbus_save_data.h"

#define VERSION					0x06
#define DRIVERTYPE			0x02

#define EEPROM_IIC_ADDR				0xA0
#define EEPROM_BASE_ADDR			0x0000
#define EEPROM_DEFAULT_ADDR		0x0100

#define MODBUS_SLAVE_ID				  7
#define MOBUS_SLAVE_TIMEOUT			10
#define MOBUS_MASTER_TIMEOUT		10
#define MODBUS_BAUDRATE				(SYSTEM_REG.BaudRate*100)

#define MOTOR_SPEED    60

#define MOTOR_ADDR					(SYSTEM_REG.Motor1Addr)
#define MOTOR_BAUDRATE				115200

#define SENSOR_C_IO					1
#define SENSOR_Z_IO					2

#define DISPATCHER_TIME_SPAN		50 //ms

#define USE_LIMIT_SENSOR			(1)


#endif
