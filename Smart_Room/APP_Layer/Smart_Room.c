/*
 * Smart_Home.c
 *
 *  Created on: Aug 29, 2024
 *      Author: Mega
 */

#define F_CPU 8000000UL
#include <util/delay.h>

#include "STD_TYPES.h"
#include "STD_MACROS.h"

#include "../MCAL_Layer/DIO/DIO_interface.h"
#include "../MCAL_Layer/EEPROM/EEPROM_interface.h"
#include "../MCAL_Layer/USART/USART_interface.h"
#include "../MCAL_Layer/TIMER/TIMER_interface.h"
#include "../MCAL_Layer/ADC/ADC_interface.h"
#include "../MCAL_Layer/GIE/GIE_interface.h"
#include "../MCAL_Layer/EXTI/EXTI_interface.h"

#include "../HAL_Layer/LED/LED_interface.h"
#include "../HAL_Layer/LM35/LM35_interface.h"
#include "../HAL_Layer/LDR/LDR_interface.h"
#include "../Hal_Layer/ClCD/CLCD_interface.h"
#include "../Hal_Layer/KPD/KPD_interface.h"

#include "SECURITY/SECURITY_interface.h"

// Used Flags
typedef struct
{
	u8 OneTimeFlag : 1;
	u8 STOP_Flag : 1;
	u8 Auto_Fan : 1;
	u8 Page_One : 1;
	u8 Lamp_One : 1;
	u8 Lamp_Two : 1;
	u8 Lamp_Three : 1;
	u8 Temp_Detect : 3;
	u8 Fan_ReturnSpeed : 1;
} Flags_structConfig;

LM35_Config LM35 = {ADC_CHANNEL0, 5, ADC_RES_10_BIT};
LDR_Config LDR1 = {ADC_CHANNEL1, 5, ADC_RES_10_BIT};
LDR_Config LDR2 = {ADC_CHANNEL2, 5, ADC_RES_10_BIT};
LDR_Config LDR3 = {ADC_CHANNEL3, 5, ADC_RES_10_BIT};

#define Time_Out 1000UL // Maximum Time Allow when not press any thing
#define LS_Pin DIO_PIN0
#define IR_Pin DIO_PIN2

volatile u8 Error_State, KPD_Press, LM35_Degree;
volatile u8 Error_Time_Out = 0, Prescaler_Falg = 0; // To count time out allow for user
extern u8 UserName[20];											// extern user name which intern with user to show on system
extern u8 UserName_Length;
volatile u8 LDR_LightPrec, LM35_Temp;
volatile u8 Timer_Counter = 0, Fan_SaveSpeed = 0;

LED_config Room_Led_1 = {DIO_PORTC, DIO_PIN5, HIGH};
LED_config Room_Led_2 = {DIO_PORTD, DIO_PIN3, HIGH};
LED_config Room_Led_3 = {DIO_PORTC, DIO_PIN4, HIGH};

// Default flags value
Flags_structConfig Flags = {1, 1, 0, 1, 0, 0, 0, 0, 0};

// Function ProtoType
void Room();
void Room_vFan();
void Room_vSetting();
void Room_Door(void);
void Auto_Fan_Control();
void ROOM_LampOne(void);
void ROOM_LampTwo(void);
void ROOM_LampThree(void);


void ISR_EXTI0_Interrupt(void); //ISR function name for external interrupt
void ISR_TIMER2_OVF_MODE(void);
void main()
{
	DIO_enumSetPinDir(DIO_PORTB, DIO_PIN0, DIO_PIN_INPUT);

	// Set Pin Direction
	LED_vInit(Room_Led_1);
	LED_vInit(Room_Led_2);
	LED_vInit(Room_Led_3);

	// Initialize CLCD Pins
	CLCD_vInit();
	// initialize ADC to Convert From Analog To digital
	ADC_vInit();
	// initialize USART to communicate with laptop with Baud Rate 9600
	USART_vInit();
	// Check EEPROM for password and username and tries left
	EEPROM_vInit();

	DIO_enumSetPinDir(DIO_PORTD, DIO_PIN5, DIO_PIN_OUTPUT);
	TIMER1_vInit();
	// set Timer2 Output PIN
	DIO_enumSetPinDir(DIO_PORTB, DIO_PIN3, DIO_PIN_OUTPUT);
	TIMER0_vInit();
	/*
	 * Initialize TIMER2 with external clock at 32.768 KHz
	 * Using division factor 128 to achieve 1 second intervals
	 */
	TIMER2_vInit();

	// Set callback function for TIMER2 overflow interrup
	TIMER_u8SetCallBack(ISR_TIMER2_OVF_MODE, TIMER2_OVF_VECTOR_ID);

	// Initialize Servo Motor
	SM_vInit();
	SM_vTimer1Degree(90);

	//SET I-Bit to enable Interrupt
	GIE_vEnable();
	//SET INT2 to execute on change on pin
	EXTI_vEnableInterrupt(EXTI_LINE0);
	EXTI_vSetSignal(EXTI_RISING_EDGE, EXTI_LINE0);
	//Set Call Back Function for ISR to INT2
	EXTI_vSetCallBack(ISR_EXTI0_Interrupt, EXTI_LINE0);
	DIO_enumSetPinDir(DIO_PORTD, DIO_PIN2, DIO_PIN_INPUT);

	while (1)
	{
		// if System is close and user want to open system
		if (Flags.OneTimeFlag == 1)
		{
			Error_Time_Out = 0;
			Flags.STOP_Flag = 1;
			CLCD_vSendString("Press enter to open system");
			do
			{
				Error_State = USART_u8ReceiveData(&KPD_Press);
				if (Error_State == OK)
				{
					if (KPD_Press == 0X0D)
					{
						break;
					}
				}
			} while (1); // go into infinite loop until press enter

			// Check username and password
			// Sign_In();

			//			CLCD_vClearScreen();
			//			// print hello message
			//			CLCD_vSetPosition(2, 7);
			//			CLCD_vSendString("Welcome ");
			//			CLCD_vSetPosition(3, ((20 - UserName_Length) / 2) + 1);
			//			CLCD_vSendString(UserName);
			//			_delay_ms(1000);
			//			Flags.OneTimeFlag = 0; // to print it one time which system is open
			Room();
		}
		else
		{
		}
	}
}

//======================================================================================================================================//
void Room()
{
	if (Flags.Page_One == 0)
	{
		CLCD_vClearScreen();
		CLCD_vSendString("Room Options : ");
		CLCD_vSetPosition(2, 1);
		CLCD_vSendString("4- Room Fan");
		CLCD_vSetPosition(3, 1);
		CLCD_vSendString("5- Room Door");
		CLCD_vSetPosition(4, 1);
		CLCD_vSendString("6- Room Setting");
		Flags.Page_One = 1;
	}
	else if (Flags.Page_One == 1)
	{
		CLCD_vClearScreen();
		CLCD_vSendString("Room Options : ");
		CLCD_vSetPosition(2, 1);
		CLCD_vSendString("1- Led1 ON/OFF");
		CLCD_vSetPosition(3, 1);
		CLCD_vSendString("2- Led2 ON/OFF");
		CLCD_vSetPosition(4, 1);
		CLCD_vSendString("3- Led3 ON/OFF");
		Flags.Page_One = 0;
	}
	else
	{
	}

	do
	{
		KPD_Press = 0XFF;
		Error_State = USART_u8ReceiveData(&KPD_Press);
		if (Error_State == OK)
		{
			switch (KPD_Press)
			{
			case '1':
				Error_Time_Out = 0;
				ROOM_LampOne();
				KPD_Press = NOTPRESSED;
				break;
			case '2':
				Error_Time_Out = 0;
				ROOM_LampTwo();
				KPD_Press = NOTPRESSED;
				break;
			case '3':
				Error_Time_Out = 0;
				ROOM_LampThree();
				KPD_Press = NOTPRESSED;
				break;
			case '4':
				Error_Time_Out = 0;
				Room_vFan();
				KPD_Press = NOTPRESSED;
				break;
			case '5':
				Error_Time_Out = 0;
				Room_Door();
				KPD_Press = NOTPRESSED;
				break;
			case '6':
				Error_Time_Out = 0;
				Room_vSetting();
				KPD_Press = NOTPRESSED;
				break;
			case '0':
				if (Flags.Page_One == 0)
				{
					CLCD_vClearScreen();
					CLCD_vSendString("Room Options : ");
					CLCD_vSetPosition(2, 1);
					CLCD_vSendString("4- Room Fan");
					CLCD_vSetPosition(3, 1);
					CLCD_vSendString("5- Room Door");
					CLCD_vSetPosition(4, 1);
					CLCD_vSendString("6- Room Setting");
					Flags.Page_One = 1;
				}
				else if (Flags.Page_One == 1)
				{
					CLCD_vClearScreen();
					CLCD_vSendString("Room Options : ");
					CLCD_vSetPosition(2, 1);
					CLCD_vSendString("1- Led1 ON/OFF");
					CLCD_vSetPosition(3, 1);
					CLCD_vSendString("2- Led2 ON/OFF");
					CLCD_vSetPosition(4, 1);
					CLCD_vSendString("3- Led3 ON/OFF");
					Flags.Page_One = 0;
				}
				else
				{
				}
				break;
			case 0x08:
				Error_Time_Out = 0;
				CLCD_vClearScreen();
				Flags.OneTimeFlag = 1; // to print it one time which system is open
				Flags.Page_One = 1;
				break;
			default:
				break;
			}
		}
		else if (Error_State == TIMEOUT_STATE)
		{
			if (Error_Time_Out == Time_Out)
			{
				if (Flags.STOP_Flag == 1)
				{
					CLCD_vClearScreen();
					CLCD_vSendString("Session Time Out");
					_delay_ms(1000);
					Flags.STOP_Flag = 0;
					CLCD_vClearScreen();
				}
				Flags.OneTimeFlag = 1;
				break;
			}
			Error_Time_Out++;
		}
	} while (KPD_Press != 0X08);
}

//======================================================================================================================================//
void ROOM_LampOne(void)
{
	CLCD_vClearScreen();
	CLCD_vSendString("Lamp1 Option :");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("1 : ON       2 : OFF");
	KPD_Press = NOTPRESSED;
	do
	{
		LDR_u8GetLightPres(&LDR1, &LDR_LightPrec);
		if (KPD_Press == '1' && LDR_LightPrec < 50)
		{
			if (Flags.Lamp_One == 0)
			{
				CLCD_vSetPosition(4, 5);
				CLCD_vSendString("  Lamp has Error");
				Flags.Lamp_One = 1;
			}
		}
		if (LDR_LightPrec > 50)
		{
			Flags.Lamp_One = 0;
			CLCD_vSetPosition(4, 5);
			CLCD_vSendString("LED Status : ON ");
			Error_State = USART_u8ReceiveData(&KPD_Press);
			if (Error_State == OK)
			{
				switch (KPD_Press)
				{
				case '1':
					break;
				case '2':
					LED_vTog(Room_Led_1);
					break;
				case 0X08:
					CLCD_vClearScreen();
					CLCD_vSendString("Room Options : ");
					CLCD_vSetPosition(2, 1);
					CLCD_vSendString("1- Led1 ON/OFF");
					CLCD_vSetPosition(3, 1);
					CLCD_vSendString("2- Led2 ON/OFF");
					CLCD_vSetPosition(4, 1);
					CLCD_vSendString("3- Led3 ON/OFF");
					break;
				default:
					break;
				}
			}
			else if (Error_State == TIMEOUT_STATE)
			{
				if (Error_Time_Out == Time_Out)
				{
					USART_u8SendData(0X0D);
					if (Flags.STOP_Flag == 1)
					{
						USART_u8SendStringSynch("Session Time Out");
						Flags.STOP_Flag = 0;
					}
					USART_u8SendData(0X0D);
					Flags.OneTimeFlag = 1;
					break;
				}
				Error_Time_Out++;
			}
		}
		else if (LDR_LightPrec < 50)
		{
			CLCD_vSetPosition(4, 5);
			if (Flags.Lamp_One == 0)
			{
				CLCD_vSendString("LED Status : OFF");
			}
			Error_State = USART_u8ReceiveData(&KPD_Press);
			if (Error_State == OK)
			{
				switch (KPD_Press)
				{
				case '1':
					LED_vTog(Room_Led_1);
					break;
				case '2':
					Flags.Lamp_One = 0;
					break;
				case 0X08:
					CLCD_vClearScreen();
					CLCD_vSendString("Room Options : ");
					CLCD_vSetPosition(2, 1);
					CLCD_vSendString("1- Led1 ON/OFF");
					CLCD_vSetPosition(3, 1);
					CLCD_vSendString("2- Led2 ON/OFF");
					CLCD_vSetPosition(4, 1);
					CLCD_vSendString("3- Led3 ON/OFF");
					break;
				default:
					break;
				}
			}
			else if (Error_State == TIMEOUT_STATE)
			{
				if (Error_Time_Out == Time_Out)
				{
					if (Flags.STOP_Flag == 1)
					{
						CLCD_vClearScreen();
						CLCD_vSendString("Session Time Out");
						_delay_ms(1000);
						Flags.STOP_Flag = 0;
						CLCD_vClearScreen();
					}
					Flags.OneTimeFlag = 1;
					break;
				}
				Error_Time_Out++;
			}
		}
		else
		{
		}
	} while (KPD_Press != 0X08);
}
//======================================================================================================================================//
void ROOM_LampTwo(void)
{
	CLCD_vClearScreen();
	CLCD_vSendString("Lamp2 Option :");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("1 : ON       2 : OFF");
	KPD_Press = NOTPRESSED;
	do
	{
		LDR_u8GetLightPres(&LDR2, &LDR_LightPrec);
		if (KPD_Press == '1' && LDR_LightPrec < 50)
		{
			if (Flags.Lamp_Two == 0)
			{
				CLCD_vSetPosition(4, 5);
				CLCD_vSendString("  Lamp has Error");
				Flags.Lamp_Two = 1;
			}
		}
		if (LDR_LightPrec > 50)
		{
			Flags.Lamp_Two = 0;
			CLCD_vSetPosition(4, 5);
			CLCD_vSendString("LED Status : ON ");
			Error_State = USART_u8ReceiveData(&KPD_Press);
			if (Error_State == OK)
			{
				switch (KPD_Press)
				{
				case '1':
					break;
				case '2':
					LED_vTog(Room_Led_2);
					break;
				case 0X08:
					CLCD_vClearScreen();
					CLCD_vSendString("Room Options : ");
					CLCD_vSetPosition(2, 1);
					CLCD_vSendString("1- Led1 ON/OFF");
					CLCD_vSetPosition(3, 1);
					CLCD_vSendString("2- Led2 ON/OFF");
					CLCD_vSetPosition(4, 1);
					CLCD_vSendString("3- Led3 ON/OFF");
					break;
				default:
					break;
				}
			}
			else if (Error_State == TIMEOUT_STATE)
			{
				if (Error_Time_Out == Time_Out)
				{
					USART_u8SendData(0X0D);
					if (Flags.STOP_Flag == 1)
					{
						USART_u8SendStringSynch("Session Time Out");
						Flags.STOP_Flag = 0;
					}
					USART_u8SendData(0X0D);
					Flags.OneTimeFlag = 1;
					break;
				}
				Error_Time_Out++;
			}
		}
		else if (LDR_LightPrec < 50)
		{
			CLCD_vSetPosition(4, 5);
			if (Flags.Lamp_Two == 0)
			{
				CLCD_vSendString("LED Status : OFF");
			}
			Error_State = USART_u8ReceiveData(&KPD_Press);
			if (Error_State == OK)
			{
				switch (KPD_Press)
				{
				case '1':
					LED_vTog(Room_Led_2);
					break;
				case '2':
					Flags.Lamp_Two = 0;
					break;
				case 0X08:
					CLCD_vClearScreen();
					CLCD_vSendString("Room Options : ");
					CLCD_vSetPosition(2, 1);
					CLCD_vSendString("1- Led1 ON/OFF");
					CLCD_vSetPosition(3, 1);
					CLCD_vSendString("2- Led2 ON/OFF");
					CLCD_vSetPosition(4, 1);
					CLCD_vSendString("3- Led3 ON/OFF");
					break;
				default:
					break;
				}
			}
			else if (Error_State == TIMEOUT_STATE)
			{
				if (Error_Time_Out == Time_Out)
				{
					if (Flags.STOP_Flag == 1)
					{
						CLCD_vClearScreen();
						CLCD_vSendString("Session Time Out");
						_delay_ms(1000);
						Flags.STOP_Flag = 0;
						CLCD_vClearScreen();
					}
					Flags.OneTimeFlag = 1;
					break;
				}
				Error_Time_Out++;
			}
		}
		else
		{
		}
	} while (KPD_Press != 0X08);
}
//======================================================================================================================================//
void ROOM_LampThree(void)
{
	CLCD_vClearScreen();
	CLCD_vSendString("Lamp3 Option :");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("1 : ON       2 : OFF");
	KPD_Press = NOTPRESSED;
	do
	{
		LDR_u8GetLightPres(&LDR3, &LDR_LightPrec);
		if (KPD_Press == '1' && LDR_LightPrec < 50)
		{
			if (Flags.Lamp_Three == 0)
			{
				CLCD_vSetPosition(4, 5);
				CLCD_vSendString("  Lamp has Error");
				Flags.Lamp_Three = 1;
			}
		}
		if (LDR_LightPrec > 50)
		{
			Flags.Lamp_Three = 0;
			CLCD_vSetPosition(4, 5);
			CLCD_vSendString("LED Status : ON ");
			Error_State = USART_u8ReceiveData(&KPD_Press);
			if (Error_State == OK)
			{
				switch (KPD_Press)
				{
				case '1':
					break;
				case '2':
					LED_vTog(Room_Led_3);
					break;
				case 0X08:
					CLCD_vClearScreen();
					CLCD_vSendString("Room Options : ");
					CLCD_vSetPosition(2, 1);
					CLCD_vSendString("1- Led1 ON/OFF");
					CLCD_vSetPosition(3, 1);
					CLCD_vSendString("2- Led2 ON/OFF");
					CLCD_vSetPosition(4, 1);
					CLCD_vSendString("3- Led3 ON/OFF");
					break;
				default:
					break;
				}
			}
			else if (Error_State == TIMEOUT_STATE)
			{
				if (Error_Time_Out == Time_Out)
				{
					USART_u8SendData(0X0D);
					if (Flags.STOP_Flag == 1)
					{
						USART_u8SendStringSynch("Session Time Out");
						Flags.STOP_Flag = 0;
					}
					USART_u8SendData(0X0D);
					Flags.OneTimeFlag = 1;
					break;
				}
				Error_Time_Out++;
			}
		}
		else if (LDR_LightPrec < 50)
		{
			CLCD_vSetPosition(4, 5);
			if (Flags.Lamp_Three == 0)
			{
				CLCD_vSendString("LED Status : OFF");
			}
			Error_State = USART_u8ReceiveData(&KPD_Press);
			if (Error_State == OK)
			{
				switch (KPD_Press)
				{
				case '1':
					LED_vTog(Room_Led_3);
					break;
				case '2':
					break;
				case 0X08:
					CLCD_vClearScreen();
					CLCD_vSendString("Room Options : ");
					CLCD_vSetPosition(2, 1);
					CLCD_vSendString("1- Led1 ON/OFF");
					CLCD_vSetPosition(3, 1);
					CLCD_vSendString("2- Led2 ON/OFF");
					CLCD_vSetPosition(4, 1);
					CLCD_vSendString("3- Led3 ON/OFF");
					break;
				default:
					break;
				}
			}
			else if (Error_State == TIMEOUT_STATE)
			{
				if (Error_Time_Out == Time_Out)
				{
					if (Flags.STOP_Flag == 1)
					{
						CLCD_vClearScreen();
						CLCD_vSendString("Session Time Out");
						_delay_ms(1000);
						Flags.STOP_Flag = 0;
						CLCD_vClearScreen();
					}
					Flags.OneTimeFlag = 1;
					break;
				}
				Error_Time_Out++;
			}
		}
		else
		{
		}
	} while (KPD_Press != 0X08);
}
//======================================================================================================================================//
void Room_vFan()
{
	CLCD_vClearScreen();
	CLCD_vSendString("Fan Control : ");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("1- Off    ");
	CLCD_vSendString("2- S 1");
	CLCD_vSetPosition(3, 1);
	CLCD_vSendString("3- S 2    ");
	CLCD_vSendString("4- S 3");
	CLCD_vSetPosition(4, 1);
	CLCD_vSendString("5- S 4    ");

	if (Flags.Auto_Fan == 1)
	{
		CLCD_vSendString("Auto Fan Control is Enabled");
	}
	do
	{
		Error_State = USART_u8ReceiveData(&KPD_Press);
		if (Error_State == OK)
		{
			switch (KPD_Press)
			{
			case '1':
				Error_Time_Out = 0;
				TIMER0_vSetCTC(0);
				_delay_ms(100);
				Timer0_vSetPrescaler(TIMER_NO_CLOCK_SOURCE);
				Prescaler_Falg = 0;
				break;
			case '2':
				Error_Time_Out = 0;
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER0_vSetCTC(63);
				break;
			case '3':
				Error_Time_Out = 0;
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER0_vSetCTC(126);
				break;
			case '4':
				Error_Time_Out = 0;
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER0_vSetCTC(189);
				break;
			case '5':
				Error_Time_Out = 0;
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER0_vSetCTC(255);
				break;
			case 0x08:
				Error_Time_Out = 0;
				Flags.Page_One = 0;
				CLCD_vClearScreen();
				CLCD_vSendString("Room Options : ");
				CLCD_vSetPosition(2, 1);
				CLCD_vSendString("1- Led1 ON/OFF");
				CLCD_vSetPosition(3, 1);
				CLCD_vSendString("2- Led2 ON/OFF");
				CLCD_vSetPosition(4, 1);
				CLCD_vSendString("3- Led3 ON/OFF");
				break;
			default:
				break;
			}
		}
		else if (Error_State == TIMEOUT_STATE)
		{
			if (Error_Time_Out == Time_Out)
			{
				if (Flags.STOP_Flag == 1)
				{
					CLCD_vClearScreen();
					CLCD_vSendString("Session Time Out");
					_delay_ms(1000);
					Flags.STOP_Flag = 0;
					CLCD_vClearScreen();
				}
				Flags.OneTimeFlag = 1;
				break;
			}
			Error_Time_Out++;
		}
	} while (KPD_Press != 0X08);
}
//======================================================================================================================================//
void Room_vSetting()
{
	CLCD_vClearScreen();
	CLCD_vSendString("Setting:");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("1- Change UserName");
	CLCD_vSetPosition(3, 1);
	CLCD_vSendString("2- Change PassWord");
	CLCD_vSetPosition(4, 1);
	CLCD_vSendString("3- Auto Fan Control");
	do
	{
		Error_State = USART_u8ReceiveData(&KPD_Press);
		if (Error_State == OK)
		{
			switch (KPD_Press)
			{
			case '1':
				EEPROM_vWrite(EEPROM_UserNameStatus, 0XFF);
				UserName_Set();
				CLCD_vClearScreen();
				CLCD_vSendString("Setting:");
				CLCD_vSetPosition(2, 1);
				CLCD_vSendString("1- Change UserName");
				CLCD_vSetPosition(3, 1);
				CLCD_vSendString("2- Change PassWord");
				CLCD_vSetPosition(4, 1);
				CLCD_vSendString("3- Auto Fan Control");
				Error_Time_Out = 0;
				break;
			case '2':
				USART_u8SendData(0X0D);
				EEPROM_vWrite(EEPROM_PassWordStatus, 0XFF);
				PassWord_Set();
				CLCD_vClearScreen();
				CLCD_vSendString("Setting:");
				CLCD_vSetPosition(2, 1);
				CLCD_vSendString("1- Change UserName");
				CLCD_vSetPosition(3, 1);
				CLCD_vSendString("2- Change PassWord");
				CLCD_vSetPosition(4, 1);
				CLCD_vSendString("3- Auto Fan Control");
				Error_Time_Out = 0;
				break;
			case '3':
				Auto_Fan_Control();
				break;
			case 0x08:
				Error_Time_Out = 0;
				CLCD_vClearScreen();
				CLCD_vSendString("Room Options : ");
				CLCD_vSetPosition(2, 1);
				CLCD_vSendString("4- Room Fan");
				CLCD_vSetPosition(3, 1);
				CLCD_vSendString("5- Room Door");
				CLCD_vSetPosition(4, 1);
				CLCD_vSendString("6- Room Setting");
				break;
			default:
				break;
			}
		}
		else if (Error_State == TIMEOUT_STATE)
		{
			if (Error_Time_Out == Time_Out)
			{
				if (Flags.STOP_Flag == 1)
				{
					CLCD_vClearScreen();
					CLCD_vSendString("Session Time Out");
					_delay_ms(1000);
					Flags.STOP_Flag = 0;
					CLCD_vClearScreen();
				}
				Flags.OneTimeFlag = 1;
				break;
			}
			Error_Time_Out++;
		}
	} while (KPD_Press != 0X08);
}

//======================================================================================================================================//
void Room_Door(void)
{
	CLCD_vClearScreen();
	CLCD_vSendString("Reception Door : ");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("1- Open      ");
	CLCD_vSendString("2- Lock");
	do
	{
		Error_State = USART_u8ReceiveData(&KPD_Press);
		if (Error_State == OK)
		{
			switch (KPD_Press)
			{
			case '1':
				SM_vTimer1Degree(0);
				Error_Time_Out = 0;
				break;
			case '2':
				SM_vTimer1Degree(90);
				Error_Time_Out = 0;
				break;
			case 0x08:
				CLCD_vClearScreen();
				CLCD_vSendString("Room Options : ");
				CLCD_vSetPosition(2, 1);
				CLCD_vSendString("1- Led1 ON/OFF");
				CLCD_vSetPosition(3, 1);
				CLCD_vSendString("2- Led2 ON/OFF");
				CLCD_vSetPosition(4, 1);
				CLCD_vSendString("3- Led3 ON/OFF");
				Flags.Page_One = 0;
				break;
			default:
				break;
			}
		}
		else if (Error_State == TIMEOUT_STATE)
		{
			if (Error_Time_Out == Time_Out)
			{
				if (Flags.STOP_Flag == 1)
				{
					CLCD_vClearScreen();
					CLCD_vSendString("Session Time Out");
					_delay_ms(1000);
					Flags.STOP_Flag = 0;
					CLCD_vClearScreen();
				}
				Flags.OneTimeFlag = 1;
				break;
			}
			Error_Time_Out++;
		}
	} while (KPD_Press != 0X08);
}
//======================================================================================================================================//
void Auto_Fan_Control()
{
	CLCD_vClearScreen();
	CLCD_vSendString("Auto Fan Control");
	CLCD_vSetPosition(2, 1);
	CLCD_vSendString("1- Open        ");
	CLCD_vSendString("2- Close");

	do
	{
		Error_State = USART_u8ReceiveData(&KPD_Press);
		if (Error_State == OK)
		{
			switch (KPD_Press)
			{
			case '1':
				Flags.Auto_Fan = 1;
				Error_Time_Out = 0;
				break;
			case '2':
				Flags.Auto_Fan = 0;
				Error_Time_Out = 0;
				break;
			case 0x08:
				USART_u8SendData(0X0D);
				Error_Time_Out = 0;
				break;
			default:
				break;
			}
		}
		else if (Error_State == TIMEOUT_STATE)
		{
			if (Error_Time_Out == Time_Out)
			{
				if (Flags.STOP_Flag == 1)
				{
					CLCD_vClearScreen();
					CLCD_vSendString("Session Time Out");
					_delay_ms(1000);
					Flags.STOP_Flag = 0;
					CLCD_vClearScreen();
				}
				Flags.OneTimeFlag = 1;
				break;
			}
			Error_Time_Out++;
		}
	} while (KPD_Press != 0x08);
}

//======================================================================================================================================//
void Fan_Speed()
{
	LM35_u8GetTemp(&LM35, &LM35_Degree);
	if (Flags.Auto_Fan == 0)
	{
		if (Flags.Fan_ReturnSpeed == 1)
		{
			Flags.Temp_Detect = 0;
			switch(Fan_SaveSpeed)
			{
			case 0:
				TIMER0_vSetCTC(0);
				Fan_SaveSpeed = 0;
				break;
			case 1:
				TIMER0_vSetCTC(63);
				Fan_SaveSpeed = 0;
				break;
			case 2:
				TIMER0_vSetCTC(126);
				Fan_SaveSpeed = 0;
				break;
			case 3:
				TIMER0_vSetCTC(189);
				Fan_SaveSpeed = 0;
				break;
			case 4:
				TIMER0_vSetCTC(255);
				Fan_SaveSpeed = 0;
				break;
			default :
				break;
			}
			Flags.Fan_ReturnSpeed = 0;
		}
	}
	else if (Flags.Auto_Fan == 1)
	{
		if (LM35_Degree < 20)
		{
			Flags.Temp_Detect = 0;
			TIMER0_vSetCTC(0);
			_delay_ms(100);
			Timer0_vSetPrescaler(TIMER_NO_CLOCK_SOURCE);
			Flags.Fan_ReturnSpeed = 1;
		}
		else if (LM35_Degree >= 20 && LM35_Degree <= 25)
		{
			if (Flags.Temp_Detect != 1)
			{
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER0_vSetCTC(50);
				Flags.Temp_Detect = 1;
				Flags.Fan_ReturnSpeed = 1;
			}
		}
		else if (LM35_Degree >= 26 && LM35_Degree <= 30)
		{
			if (Flags.Temp_Detect != 2)
			{
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER0_vSetCTC(100);
				Flags.Temp_Detect = 2;
				Flags.Fan_ReturnSpeed = 1;
			}
		}
		else if (LM35_Degree >= 31 && LM35_Degree <= 35)
		{

			if (Flags.Temp_Detect != 3)
			{
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER0_vSetCTC(150);
				Flags.Temp_Detect = 3;
				Flags.Fan_ReturnSpeed = 1;
			}
		}
		else if (LM35_Degree >= 36 && LM35_Degree <= 40)
		{
			if (Flags.Temp_Detect != 4)
			{
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER0_vSetCTC(200);
				Flags.Temp_Detect = 4;
				Flags.Fan_ReturnSpeed = 1;
			}
		}
		else if (LM35_Degree >= 41 && LM35_Degree <= 45)
		{

			if (Flags.Temp_Detect != 5)
			{
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER0_vSetCTC(255);
				Flags.Temp_Detect = 5;
				Flags.Fan_ReturnSpeed = 1;
			}
		}
		else
		{

		}
	}
}



//======================================================================================================================================//

void ISR_EXTI0_Interrupt(void)
{
	u8 LS_Status= 0;
	DIO_enumReadPinVal(DIO_PORTB, LS_Pin, &LS_Status);
	if (LS_Status == 1)
	{

	}
	else if (LS_Status == 0)
	{
		//Turn on Led one when it is Dark
		LDR_LightPrec = 0XFF;
		LDR_u8GetLightPres(&LDR1, &LDR_LightPrec);
		if (LDR_LightPrec < 50)
		{
			LED_vTog(Room_Led_1);
		}
		else
		{

		}
		//Turn on Led two when it is Dark
		LDR_LightPrec = 0XFF;
		LDR_u8GetLightPres(&LDR2, &LDR_LightPrec);
		if (LDR_LightPrec < 50)
		{
			LED_vTog(Room_Led_2);
		}
		else
		{

		}
		//Turn on Led three when it is Dark
		LDR_LightPrec = 0XFF;
		LDR_u8GetLightPres(&LDR3, &LDR_LightPrec);
		if (LDR_LightPrec < 50)
		{
			LED_vTog(Room_Led_3);
		}
		else
		{

		}
	}
	else
	{

	}


}
//======================================================================================================================================//
void ISR_TIMER2_OVF_MODE()
{
	Timer_Counter++;
	u8 IR_PinRead = 0;
	Fan_Speed();

	DIO_enumReadPinVal(DIO_PORTD, IR_Pin, &IR_PinRead);
	if (IR_PinRead == 1)
	{
		Timer_Counter = 0;
	}
	else if (IR_PinRead == 0 && Timer_Counter == 3)
	{
		//Turn on Led one when it is Dark
		LDR_LightPrec = 0XFF;
		LDR_u8GetLightPres(&LDR1, &LDR_LightPrec);
		if (LDR_LightPrec > 50)
		{
			LED_vTog(Room_Led_1);
		}
		else
		{

		}
		//Turn on Led two when it is Dark
		LDR_LightPrec = 0XFF;
		LDR_u8GetLightPres(&LDR2, &LDR_LightPrec);
		if (LDR_LightPrec > 50)
		{
			LED_vTog(Room_Led_2);
		}
		else
		{

		}
		//Turn on Led three when it is Dark
		LDR_LightPrec = 0XFF;
		LDR_u8GetLightPres(&LDR3, &LDR_LightPrec);
		if (LDR_LightPrec > 50)
		{
			LED_vTog(Room_Led_3);
		}
		else
		{

		}
	}


}
