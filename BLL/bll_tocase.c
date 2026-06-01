#include "bll_tocase.h"
#include "bll_adapter.h"
#include "bll_motor.h"
#include "modbus_master.h"
#include "config.h"
#include "delay.h"
#include "usart.h"
#include "gpio.h"
#include "timer.h"
#include "bll_backzero.h"
#include "jexception.h"
#include "exti.h"

static CommonStateFlag_Type tocase_flag = CSF_Idel; // 声明原来的状态
volatile u8 running_flag = 0;
volatile u8 has_zero_flag = 0;
volatile u8 last_target = 0;
extern volatile u8 wait_zero_flag;
extern volatile u8 zero_trigger_flag;
extern volatile u32 zero_trigger_time;
extern volatile s32 zero_turn_num;
extern volatile s32 zero_encoder_num;
static vu8 swtich_flag;
volatile vu8 swtich4_count = 0;
volatile vu8 counter_dir = 0;
static vu8 target;
vu8 special_case;
static vu8 div10 = 0;
static vu8 div20 = 0;
static vu8 kind;
static vu8 fast_move_flag;
static vu8 slow_move_flag;
static vu8 counter_flag = 0;
static vu32 counter_time = 0;
extern volatile ModBusState Master_State;
static vu32 switch_trigger_time = 0;
static BitAction ELECTRIAL_LEVEL = Bit_SET;

//==================================================================
// TODO: 测试完毕后，留意各种delay，能缩短的要缩短

// 对应了4个口的位置参数，用于升降找到具体格口的位置
static const s32 Special_Positions[] =
	{
		0,
		-23650 + 300 + 1500 - 300, // 0xFF(255)、出证口
		-35550 + 500 + 1200 - 200, // 0xFE(254)、入证口
		-38900 + 300 + 1200 - 200, // 0xFD(253)、入卡口
		0						   // 0xFC(252)、批量口
};

static s16 sp_final_offset[5] = {0, 170, 450 - 30, 360, 150}; // 这里是特殊位置的最终偏移

#define FINAL_OFFSET_in 400 // 入证口，入卡偏移

#define SLOW_ACCE (5000)
#define SLOW_DECE (5000)
#define SLOW_SPEED (1000)

#define FAST_ACCE (80000)
#define FAST_DECE (50000)
#define FAST_SPEED (40000) // 25000-30000-33000

#define FINAL_OFFSET (-80)	   // 证最终偏移(正数向下，负数向上) -80
#define FINAL_OFFSET_CARD (35) // 卡最终偏移(正数向下，负数向上) 28 to 35 to 55 to 70 to 20

#define MAX_SLOW_TIME (1000)

// 证的KGB
#define K_BOOK (933 + 0.6)
#define G_BOOK (460 + 4)
#define B_BOOK (20 + 5 + 5)

// 卡的KGB
#define K_CARD (465)
#define G_CARD (500)
#define B_CARD (10)

#define BUG_80_OFFSET (K_CARD) // 465 to 400

// 当参数4发送4或者5时，触发以下的盘点专用偏移
#define CARD_BUG_OFFSET (-102) // 正数向上，负数向下
#define PP_BUG_OFFSET (-190)   // 正数向上，负数向下

#define MOVE_DELAY (20) // 目前只做了证格口运动时的延时50-30-20

// 当参数4发送10或者11时，触发以下的插入后移动
#define PP_DOWN (160 + 10 + 10) // 证插入后下移距离(正数向下，负数向上) 155-160-165
#define CARD_DOWN (70)			// 卡插入后下移距离(正数向下，负数向上) 22 to 5 to 20 to 40 to 25 to 70

//==================================================================

#define SET_PROCESS(h, l) (REALTIME_REG.WorkingProcess = (((h) << 8) | (l))) // REALTIME_REG是一个结构体，里面有WorkingProcess这个变量
																			 // 可用于监控护照柜具体执行到哪一步，方便查错调整

// 清除flag状态,清成空闲状态Idel
void BLL_ToCase_ClearFlag(void)
{
	tocase_flag = CSF_Idel;
}

static void Clear_Manual_Flags(void)
{
	special_case = 0;
	swtich_flag = 0;
	fast_move_flag = 0;
	slow_move_flag = 0;
}

// 延时xms,使用TIM5定时器计数作为延时的基准
void MyDelay(u16 ms)
{
	u32 now = ReadTick();
	while (TickSpan(now) < ms)
		;
}

// vu16 _motor_sate = 0;
// vu16 states[100],xxx = 0;
#define WAIT_MOTOR_STOP(span, n, label)         \
	{                                           \
		vu16 _motor_sate = 0, _retry = 0;       \
		do                                      \
		{                                       \
			delay_ms(300);                      \
			if (_retry++ >= n)                  \
			{                                   \
				*err = Failure_Timeout;         \
				has_zero_flag = 0;              \
				goto label;                     \
			}                                   \
			if (Check_LimitTriggered())         \
			{                                   \
				Brake();                        \
				*err = (Failure_Limit);         \
				goto label;                     \
			}                                   \
			_motor_sate = Check_Status();       \
		} while ((_motor_sate & 0x01) != 0x01); \
	}

static void WaitMotorStop(u16 span, u16 n)
{
	u16 _motor_sate = 0, _retry = 0;
	do
	{
		delay_ms(span + 100);
		if (_retry++ >= n)
		{
			throw(Failure_Timeout); //_motor_sate一直不是理想状态，超时了
		}
		if (Check_LimitTriggered()) // PC7或者PC8被触发了，消抖3ms，如果还是有触发，说明触发限位了
		{
			Brake();
			throw(Failure_Limit);
		}

		_motor_sate = Check_Status();
	} while ((_motor_sate & 0x01) != 0x01);
}

__forceinline u8 Check_Switch(u8 kind, u8 special_case)
{
	if (special_case == 0)
	{
		//		return (XIN(2) == SET && (kind ? (XIN(3) == SET) : (XIN(1) == SET)));
		if (kind == 0)
		{
			return (XIN(1) == SET && (XIN(2) == SET)); // PC2和PC3是否都为高电平？
		}
		else
		{
			return (XIN(1) == SET && (XIN(2) == SET));
		}
	}
	else
	{
		return (XIN(4) == SET); // 如果special_case不为0，查看PC5是否是高电平？
	}
}
// 慢速修正，会修到两个传感器PC2,3都置位或者超时为止
static u8 CaseSlowMove(u32 acc, u32 dece, u32 speed, s32 maxdistance, u16 maxtime, u8 kind)
{
	SET_PROCESS(maxdistance < 0 ? -1 : 1, 26); // 往REALTIME_REG的WorkingProcess写入这两个值
	slow_move_flag = 1;
	swtich_flag = 0;
	BLL_Motor_AD_RelativeMove(maxdistance, acc, dece, speed); // 电机开始修正运动

	u32 start = ReadTick();
	while (1)
	{
		if (swtich_flag) // slowmove_flag置1后中断会进函数，当PC2,3都高电平的时候会把switch_flag置1，进而当高电平稳定两秒后再停止
		{
			if (Check_Switch(kind, special_case))
			{
				if (TickSpan(switch_trigger_time) >= 2) // PC2,3都是高电平稳定2ms
				{
					Brake();
					slow_move_flag = 0;
					delay_ms(100);
					if (special_case)
					{
						if (Read_Switch(4) == SET) // PC5是否置位
						{
							if (counter_dir == 0)
							{
								swtich4_count++;
							}
							else
							{
								swtich4_count--;
							}
						}
						else
						{
							has_zero_flag = 0;
						}
					}
					return 1;
				}
			}
			else
			{
				swtich_flag = 0;
				slow_move_flag = 1;
			}
		}
		if (TickSpan(start) > maxtime) // 超时就直接刹停
		{
			Brake();
			slow_move_flag = 0;
			return 0;
		}
	}
}

CommonStateFlag_Type BLL_ToCase_Execute(ParamShadow_Type params, u8 *err)
{
	if (tocase_flag == CSF_Idel)
	{
		// Step0：初始化各种标志位等
		tocase_flag = CSF_Working;
		running_flag = 1;
		USART_ClearFlag(MOTOR_USART, USART_FLAG_TC); // 清除USART状态标志位
		Clear_Manual_Flags();
		SET_PROCESS(0, 1);
		// Step1：进行回零
		if (has_zero_flag == 0)
		{
			BackZero();
			MyDelay(800); // 1000-800
		}

		if (params.Param4 == 10 || params.Param4 == 11) // 如果参数4是10或11，触发证/卡插入后的下移动作
		{
			s32 move = ((params.Param4 == 10) ? PP_DOWN : CARD_DOWN);

			try
			{
				BLL_Motor_AD_RelativeMove(move, SLOW_ACCE, SLOW_DECE, SLOW_SPEED); // 先进行下移动作
				WaitMotorStop(100, 4000);
			}
			catch
			{
				*err = GetLastError();
				goto die;
			}

			goto die;
		}

		// 第一个参数是第几行，最多支持128行，超过这个行数就无法执行证格口的升降移动逻辑
		if ((params.Param1 >> 7) == 0 && (params.Param4 == 0 || params.Param4 == 5 || params.Param4 == 30 || params.Param4 == 40)) // 证格口
		{
			target = params.Param1 & 0x00ff; // 相当于只看后8位，但是第8位是0，因此只看后7位
			div10 = (target - 1) / 10;		 // 每过10个格口会有一段的连接处长度，需要考虑在内

			s32 pp_bug = 0;
			if (params.Param4 == 5) // 5是证，4是卡
			{
				pp_bug = PP_BUG_OFFSET;
			}

			kind = 0; // kind=？不重要，执行的逻辑是一样的
			// Step2：快速相对位移模式
			fast_move_flag = 1;
			s32 sub1 = (target - 1) * K_BOOK + div10 * G_BOOK + B_BOOK; // 关键逻辑，把整列的3d打印柜用target表示行号，每过10行要加一个G_BOOK的修正，以及最后B_BOOK的修正
			SET_PROCESS(0, 2);

			if (params.Param4 == 40) // 参数4=40时，不做快速运动，直接进行慢速修正(证)
				goto SLOW_BOOK;
			u8 move_rst = BLL_Motor_AD_AbsoluteMove(-sub1, FAST_ACCE, FAST_DECE, FAST_SPEED);
			SET_PROCESS(0, 3);
			// MyDelay(500);
			WAIT_MOTOR_STOP(100, 3000, die); // 100ms查一次，查300次不行就超时
			SET_PROCESS(0, 4);

			fast_move_flag = 0;

			if (move_rst)
			{
				*err = 8;
				goto die;
			}

			MyDelay(MOVE_DELAY);

			if (params.Param4 == 30) // 参数4=30时，只执行快速运动(证) 直接不修了，应该是用来看格口对的准不准的
				goto die;
		SLOW_BOOK:
			// goto die; //暂时不用修正

			// Step3：判断位移模式结果，决定如何修正
			// 如果两个红外传感器都置位，说明已经对上了，可以直接进行最终偏移修正
			if ((Read_Switch(1) == SET) && (Read_Switch(2) == SET)) // 如果PC2和PC3都置位
			{
				delay_ms(2); // 消抖
				if ((Read_Switch(1) == SET) && (Read_Switch(2) == SET))
				{
					MyDelay(MOVE_DELAY);
					SET_PROCESS(0, 5);
					BLL_Motor_AD_RelativeMove(FINAL_OFFSET - pp_bug, SLOW_ACCE, SLOW_DECE, SLOW_SPEED);
					SET_PROCESS(0, 6);
					WAIT_MOTOR_STOP(100, 5000, die); // 100ms查一次，查300次不行就超时
					SET_PROCESS(0, 7);
					goto die;
				}
			}
			// 如果有传感器没对上，就先上修再下修，修成功了之后再进行最终偏移
			else
			{
				try
				{
					if (CaseSlowMove(SLOW_ACCE, SLOW_ACCE * 10, SLOW_SPEED, -500, MAX_SLOW_TIME, 0)) // 如果上修成功
					{
						MyDelay(MOVE_DELAY);
						SET_PROCESS(0, 8);
						BLL_Motor_AD_RelativeMove(FINAL_OFFSET - pp_bug, SLOW_ACCE, SLOW_DECE, SLOW_SPEED); // 最终修正
						SET_PROCESS(0, 9);
						WaitMotorStop(100, 4000);
						SET_PROCESS(0, 10);
						goto die;
					}
					else if (CaseSlowMove(SLOW_ACCE, SLOW_ACCE * 10, SLOW_SPEED, 1000, MAX_SLOW_TIME * 2, 0)) // 如果下修成功
					{
						MyDelay(MOVE_DELAY);
						SET_PROCESS(0, 11);
						BLL_Motor_AD_RelativeMove(FINAL_OFFSET - pp_bug, SLOW_ACCE, SLOW_DECE, SLOW_SPEED); // 最终修正
						SET_PROCESS(0, 12);
						WaitMotorStop(100, 4000);
						SET_PROCESS(0, 13);
						goto die;
					}
					else
					{
						throw(Failure_Aim);
					}
				}
				catch
				{
					*err = GetLastError();
					goto die;
				}
			}
		}
		// 这里是输入的卡口没到240以上，就不是特殊位置，就走正常的卡格口升降
		if ((params.Param1 & 0xf0) != 0xf0 && (params.Param4 == 1 || params.Param4 == 4 || params.Param4 == 21 || params.Param4 == 31 || params.Param4 == 41)) // 卡格口
		{
			target = params.Param1 & 0x00ff;
			div10 = (target - 1) / 20;

			// 80号格口bug
			u8 bug80 = 0;
			if (params.Param4 == 21) // 参数21触发80号格口的修正
			{
				bug80 = 1;
				target = target - 1;
			}

			kind = 1;
			// Step2：快速相对位移模式
			fast_move_flag = 1;
			s32 sub2 = (target - 1) * K_CARD + div10 * G_CARD + B_CARD;
			SET_PROCESS(0, 14);
			if (params.Param4 == 41) // 参数4=41时，不做快速运动，直接进行慢速修正(卡)
				goto SLOW_CARD;
			u8 move_rst = BLL_Motor_AD_AbsoluteMove(-sub2, FAST_ACCE, FAST_DECE, FAST_SPEED);
			SET_PROCESS(0, 15);
			WAIT_MOTOR_STOP(100, 5000, die); // 100ms查一次，查300次不行就超时
			SET_PROCESS(0, 16);
			fast_move_flag = 0;

			if (move_rst)
			{
				*err = 8;
				goto die;
			}

			MyDelay(MOVE_DELAY);

			if (params.Param4 == 31) // 参数4=31时，只执行快速运动(卡)
				goto die;
		SLOW_CARD:
			//			goto die;

			// Step3：判断位移模式结果，决定如何修正
			//			if((Read_Switch(2) == SET) && (Read_Switch(3) == SET))
			//			{
			//				delay_ms(2);
			//				if((Read_Switch(2) == SET) && (Read_Switch(3) == SET))
			//				{
			//					goto die;
			//				}
			//			}
			if ((Read_Switch(2) == SET) && (Read_Switch(1) == SET))
			{
				delay_ms(2);
				if ((Read_Switch(2) == SET) && (Read_Switch(1) == SET))
				{
					if (bug80) // 80bug
					{
						SET_PROCESS(0, 17);
						BLL_Motor_AD_RelativeMove(-BUG_80_OFFSET, SLOW_ACCE, SLOW_DECE, SLOW_SPEED); // 80号卡格口修正
						SET_PROCESS(0, 18);
						WaitMotorStop(100, 4000);
						SET_PROCESS(0, 19);
					}
					if (params.Param4 == 4)
					{
						SET_PROCESS(0, 20);
						BLL_Motor_AD_RelativeMove(-CARD_BUG_OFFSET, SLOW_ACCE, SLOW_DECE, SLOW_SPEED);
						SET_PROCESS(0, 21);
						WaitMotorStop(100, 4000);
						SET_PROCESS(0, 22);
					}
					MyDelay(MOVE_DELAY);
					SET_PROCESS(0, 23);
					BLL_Motor_AD_RelativeMove(FINAL_OFFSET_CARD, SLOW_ACCE, SLOW_DECE, SLOW_SPEED); // 最终修正
					SET_PROCESS(0, 24);
					WaitMotorStop(100, 4000);
					SET_PROCESS(0, 25);
					goto die;
				}
			}
			else
			{
				try
				{
					if (CaseSlowMove(SLOW_ACCE, SLOW_ACCE * 10, SLOW_SPEED, -300, MAX_SLOW_TIME, 1)) // 如果上修成功
					{
						// 卡就不进行最终修正了
						if (bug80) // 80bug
						{
							SET_PROCESS(0, 27);
							BLL_Motor_AD_RelativeMove(-BUG_80_OFFSET, SLOW_ACCE, SLOW_DECE, SLOW_SPEED); // 80号卡格口修正
							SET_PROCESS(0, 28);
							WaitMotorStop(100, 4000);
							SET_PROCESS(0, 29);
						}
						if (params.Param4 == 4)
						{
							SET_PROCESS(0, 30);
							BLL_Motor_AD_RelativeMove(-CARD_BUG_OFFSET, SLOW_ACCE, SLOW_DECE, SLOW_SPEED);
							SET_PROCESS(0, 31);
							WaitMotorStop(100, 4000);
							SET_PROCESS(0, 32);
						}
						MyDelay(MOVE_DELAY);
						SET_PROCESS(0, 33);
						BLL_Motor_AD_RelativeMove(FINAL_OFFSET_CARD, SLOW_ACCE, SLOW_DECE, SLOW_SPEED); // 最终修正
						SET_PROCESS(0, 34);
						WaitMotorStop(100, 4000);
						SET_PROCESS(0, 35);
						goto die;
					}
					else if (CaseSlowMove(SLOW_ACCE, SLOW_ACCE * 10, SLOW_SPEED, 600, MAX_SLOW_TIME * 2, 1)) // 如果下修成功
					{
						// 卡就不进行最终修正了
						if (bug80) // 80bug
						{
							SET_PROCESS(0, 36);
							BLL_Motor_AD_RelativeMove(-BUG_80_OFFSET, SLOW_ACCE, SLOW_DECE, SLOW_SPEED); // 80号卡格口修正
							SET_PROCESS(0, 37);
							WaitMotorStop(100, 4000);
							SET_PROCESS(0, 38);
						}
						if (params.Param4 == 4)
						{
							SET_PROCESS(0, 39);
							BLL_Motor_AD_RelativeMove(-CARD_BUG_OFFSET, SLOW_ACCE, SLOW_DECE, SLOW_SPEED);
							SET_PROCESS(0, 40);
							WaitMotorStop(100, 4000);
							SET_PROCESS(0, 41);
						}
						MyDelay(MOVE_DELAY);
						SET_PROCESS(0, 42);
						BLL_Motor_AD_RelativeMove(FINAL_OFFSET_CARD, SLOW_ACCE, SLOW_DECE, SLOW_SPEED); // 最终修正
						SET_PROCESS(0, 43);
						WaitMotorStop(100, 4000);
						SET_PROCESS(0, 44);
						goto die;
					}
					else
					{
						throw(Failure_Aim);
					}
				}
				catch
				{
					*err = GetLastError();
					goto die;
				}
			}
		}
		if ((params.Param1 & 0xf0) == 0xf0) // 特殊位置 分别是252~255
		{
			special_case = 0 - (u8)params.Param1;

			// Step2：快速相对位移模式
			//			if(special_case - last_target > 0)
			//			{
			//				counter_dir = 0;
			//				BLL_Motor_AD_RelativeMove(-500,FAST_ACCE,FAST_DECE,FAST_SPEED);
			//				WAIT_MOTOR_STOP(200,3000,die);	//100ms查一次，查300次不行就超时
			//			}
			//			else if(special_case - last_target < 0)
			//			{
			//				counter_dir = 1;
			//				swtich4_count--;
			//				BLL_Motor_AD_RelativeMove(500,FAST_ACCE,FAST_DECE,FAST_SPEED);
			//				WAIT_MOTOR_STOP(200,3000,die);	//100ms查一次，查300次不行就超时
			//			}
			//			else
			//			{
			//				goto die;
			//			}

			last_target = special_case;

			// Step2：快速相对位移模式
			fast_move_flag = 1;
			counter_flag = 0;
			SET_PROCESS(0, 45);
			u8 move_rst = BLL_Motor_AD_AbsoluteMove(Special_Positions[special_case], FAST_ACCE, FAST_DECE, FAST_SPEED);
			SET_PROCESS(0, 46);
			{
				vu16 _motor_sate = 0, _retry = 0;
				u32 starttime = 0;
				do
				{
					if (counter_flag == 1 && TickSpan(counter_time) >= 15)
					{
						counter_flag = 0;
						if (Read_Switch(4) == ELECTRIAL_LEVEL)
						{
							if (counter_dir == 0)
							{
								swtich4_count++;
							}
							else
							{
								swtich4_count--;
							}
						}
					}
					if (TickSpan(starttime) >= 300)
					{
						starttime = ReadTick();
						CHECK_LIMIT_STOP(err, die);
						_motor_sate = Check_Status();
						if (_retry++ >= 4000)
						{
							*err = Failure_Timeout;
							has_zero_flag = 0;
							goto die;
						}
					}
				} while ((_motor_sate & 0x01) != 0x01);
			}
			fast_move_flag = 0;

			if (move_rst)
			{
				*err = 8;
				goto die;
			}

			MyDelay(100);

			//			goto die;

			// Step3：判断位移模式结果，决定如何修正
			if ((Read_Switch(4) == SET))
			//			if((Read_Switch(4) == SET) && (swtich4_count == special_case))
			{
				delay_ms(2);
				if (Read_Switch(4) == SET)
				{
					MyDelay(MOVE_DELAY);
					SET_PROCESS(0, 47);
					BLL_Motor_AD_RelativeMove(sp_final_offset[special_case], SLOW_ACCE, SLOW_DECE, SLOW_SPEED); // 最终修正
					SET_PROCESS(0, 48);
					WaitMotorStop(100, 4000);
					SET_PROCESS(0, 49);
					goto die;
				}
			}
			else
			{
				try
				{
					if (CaseSlowMove(SLOW_ACCE, SLOW_ACCE * 10, SLOW_SPEED, -2000, MAX_SLOW_TIME, 0)) // 如果上修成功
					{
						MyDelay(MOVE_DELAY);
						SET_PROCESS(0, 50);
						BLL_Motor_AD_RelativeMove(sp_final_offset[special_case], SLOW_ACCE, SLOW_DECE, SLOW_SPEED); // 最终修正
						SET_PROCESS(0, 51);
						WaitMotorStop(100, 4000);
						SET_PROCESS(0, 52);
						goto die;
					}
					else if (CaseSlowMove(SLOW_ACCE, SLOW_ACCE * 10, SLOW_SPEED, 4000, MAX_SLOW_TIME * 2, 0)) // 如果下修成功
					{
						MyDelay(MOVE_DELAY);
						SET_PROCESS(0, 53);
						BLL_Motor_AD_RelativeMove(sp_final_offset[special_case], SLOW_ACCE, SLOW_DECE, SLOW_SPEED); // 最终修正
						SET_PROCESS(0, 54);
						WaitMotorStop(100, 4000);
						SET_PROCESS(0, 55);
						goto die;
					}
					else
					{
						throw(Failure_Aim);
					}
				}
				catch
				{
					*err = GetLastError();
					goto die;
				}
				//				if(swtich4_count == (special_case - 1))
				//				{
				//					slow_move_flag = 1;
				//					counter_dir = 0;
				//					if(CaseSlowMove(SLOW_ACCE,SLOW_ACCE*10,SLOW_SPEED,-0x0fffffff,MAX_SLOW_TIME,1))	//如果上修成功
				//					{
				//						goto die;
				//					}
				//				}
				//				else if(swtich4_count == special_case)
				//				{
				//					slow_move_flag = 1;
				//					counter_dir = 1;
				//					swtich4_count++;
				//					if(CaseSlowMove(SLOW_ACCE,SLOW_ACCE*10,SLOW_SPEED,0x0fffffff,MAX_SLOW_TIME,1)) //如果下修成功
				//					{
				//						goto die;
				//					}
				//				}
				//				else
				//				{
				//					*err = Failure_Actuator;
				//					has_zero_flag = 0;
				//					goto die;
				//				}
			}
		}
	}

// 最终程序出口
die:
	tocase_flag = CSF_Finished;
	running_flag = 0;
	Clear_Manual_Flags();
	SET_PROCESS(0, 56);
	return tocase_flag;
}

#define EXTI_DEBOUNCE_US 10

void SWTICH1_INT_HANDLER(void) // 通过PC2的电平变化来触发
{
	if (tocase_flag == CSF_Working)
	{
		if (slow_move_flag)
		{
			if (special_case == 0)
			{
				delay_us(EXTI_DEBOUNCE_US);
				if (kind == 0)
				{
					if ((Read_Switch(1) == SET) && (Read_Switch(2) == SET))
					{
						swtich_flag = 1;
						switch_trigger_time = ReadTick();
						slow_move_flag = 0;
					}
				}
				if (kind == 1)
				{
					if ((Read_Switch(2) == SET) && (Read_Switch(1) == SET))
					{
						swtich_flag = 1;
						switch_trigger_time = ReadTick();
						slow_move_flag = 0;
					}
				}
			}
		}
	}
}

void SWTICH2_INT_HANDLER(void) // 通过PC3的电平变化来触发
{
	if (tocase_flag == CSF_Working)
	{
		if (slow_move_flag)
		{
			if (special_case == 0)
			{
				delay_us(EXTI_DEBOUNCE_US);
				if (kind == 0)
				{
					if ((Read_Switch(2) == SET) && (Read_Switch(1) == SET))
					{
						swtich_flag = 1;
						switch_trigger_time = ReadTick();
						slow_move_flag = 0;
					}
				}
				if (kind == 1)
				{
					if ((Read_Switch(2) == SET) && (Read_Switch(1) == SET))
					{
						swtich_flag = 1;
						switch_trigger_time = ReadTick();
						slow_move_flag = 0;
					}
				}
			}
		}
	}
}

void SWTICH3_INT_HANDLER(void) // PC4
{
	if (tocase_flag == CSF_Working)
	{
		if (slow_move_flag)
		{
			if (special_case == 0)
			{
				delay_us(EXTI_DEBOUNCE_US);
				if (kind == 1)
				{
					if ((Read_Switch(3) == SET) && (Read_Switch(2) == SET))
					{
						swtich_flag = 1;
						switch_trigger_time = ReadTick();
						slow_move_flag = 0;
					}
				}
			}
		}
	}
}

void SWTICH4_INT_HANDLER(void) // PC5
{
	if (tocase_flag == CSF_Working)
	{
		if (special_case != 0)
		{
			delay_us(EXTI_DEBOUNCE_US);
			if (Read_Switch(4) == ELECTRIAL_LEVEL)
			{
				if (fast_move_flag)
				{
					if (counter_flag == 0)
					{
						counter_flag = 1;
						counter_time = ReadTick();
					}
				}
				if (slow_move_flag)
				{
					swtich_flag = 1;
					switch_trigger_time = ReadTick();
					slow_move_flag = 0;
				}
			}
		}
	}
}
