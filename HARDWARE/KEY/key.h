#ifndef __KEY_H
#define __KEY_H
#include "sys.h"

/*按键的方式，通过直接操作库函数方式读取IO*/
#define KEY0 		gpio_input_bit_get(GPIOE, GPIO_PIN_4) //PE4
#define KEY1 		gpio_input_bit_get(GPIOE, GPIO_PIN_3) //PE3
#define KEY2 		gpio_input_bit_get(GPIOE, GPIO_PIN_2) //PE2
#define WK_UP 	        gpio_input_bit_get(GPIOA, GPIO_PIN_0) //PA0

/*下面方式，通过位带操作方式读取IO*/
/*
#define KEY0 		PEin(4)   	//PE4
#define KEY1 		PEin(3)		//PE3
#define KEY2 		PEin(2)		//P32
#define WK_UP 	PAin(0)		//PA0
*/

#define KEY0_PRES 	1
#define KEY1_PRES	2
#define KEY2_PRES	3
#define WKUP_PRES   4

void KEY_Init(void);
u8 KEY_Scan(u8);

#endif
