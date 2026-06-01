#ifndef __MODBUS_SLAVE_H__
#define __MODBUS_SLAVE_H__

#include "sys.h"
#include "common.h"
#include "modbus_save_data.h"

//Start of configuration section

#define FUN_START_TIMER()		TIM_Cmd(MODBUS_SLAVE_TIM, ENABLE)
#define FUN_RESET_TIMER()		TIM_SetCounter(MODBUS_SLAVE_TIM, 0)
#define FUN_SEND_DATA(d,l)	USART1_SendWithDMA(d,l)
#define FUN_485_DIR(d)			__nop()

//End of configuration section

#define MODBUS_ADDR	 MODBUS_SLAVE_ID
#define DIR_485_READONLY	0
#define DIR_485_WRITEONLY	1
//#define MODBUS_REG_COUNT	0x600
#define MODBUS_BUFF_LEN		1024

typedef enum ModBusState
{
	MODBUS_IDLE,
	MODBUS_RECING,
	MODBUS_TIMEOUT,
	MODBUS_HANDLING,
	MODBUS_SENDING,
	MODBUS_WAIT_RESPONSE
}ModBusState;

typedef enum ModBusFailCode_Type
{
	MODBUS_OK,
	MODBUS_ERR_Func = 0x01,
	MODBUS_ERR_Reg = 0x02,
	MODBUS_ERR_Failure = 0x04,
	MODBUS_ERR_Confirm = 0x05,
	MODBUS_ERR_Busy = 0x07,
	MODBUS_ERR_CRC = 0x08,
	MODBUS_ERR_Addr = 0x0A,
	MODBUS_ERR_NoResponse = 0x0B
}ModBusFailCode_Type;


#if BYTE_ENDIAN==0
	typedef union Byte16ConvertUnion
	{
		u16 data16;
		struct
		{
			u8 LOW;
			u8 HIGH;
		}data8;
	}Byte16ConvertUnion;
#else
	typedef union Byte16ConvertUnion
	{
		u16 data16;
		struct
		{
			u8 HIGH;
			u8 LOW;
		}data8;
	}Byte16ConvertUnion;
#endif

#define Data16To8(d,h,l) { \
	u16 t = (d); \
	(h)=((Byte16ConvertUnion*)&t)->data8.HIGH; \
	(l)=((Byte16ConvertUnion*)&t)->data8.LOW; } 


#define Data8To16(d,h,l) { \
	u16* t = &(d); \
	((Byte16ConvertUnion*)t)->data8.HIGH=(h); \
	((Byte16ConvertUnion*)t)->data8.LOW=(l); }

//#define MODBUS_READ_REG(r)			(Reg_4x[(r)])
//#define MODBUS_WRITE_REG(r,d)		(Reg_4x[(r)]=(d))
	
//extern vu16 Reg_4x[];	
	
void ModBusRecByte(u8 data);
void ModBus_SetTimeout(void);
void Modbus_ResetToIdle(void);
void ModBus_Pool(void);

#endif


	
	
	

	
