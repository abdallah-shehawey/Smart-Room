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

#include "../HAL_Layer/LED/LED_interface.h"
#include "../HAL_Layer/LM35/LM35_interface.h"
#include "../HAL_Layer/LDR/LDR_interface.h"
#include "../Hal_Layer/ClCD/CLCD_interface.h"
#include "../Hal_Layer/KPD/KPD_interface.h"

#include "SECURITY/SECURITY_interface.h"

// Used Flags
typedef struct
{
	unsigned char OneTimeFlag : 1;
	unsigned char STOP_Flag : 1;
	unsigned char Auto_Fan : 1;
	unsigned char Page_One : 1;
	unsigned char Lamp_One : 1;
	unsigned char Lamp_Two : 1;
	unsigned char Lamp_Three : 1;
} Flags_structConfig;

LM35_Config LM35 = {ADC_CHANNEL0, 5, ADC_RES_10_BIT};
LDR_Config LDR1 = {ADC_CHANNEL1, 5, ADC_RES_10_BIT};
LDR_Config LDR2 = {ADC_CHANNEL2, 5, ADC_RES_10_BIT};
LDR_Config LDR3 = {ADC_CHANNEL3, 5, ADC_RES_10_BIT};

#define Time_Out 1000UL // Maximum Time Allow when not press any thing

volatile u8 Error_State, KPD_Press, SPI_Recieve;
volatile u8 Error_Time_Out = 0, Prescaler_Falg = 0; // To count time out allow for user
extern u8 UserName[20];											// extern user name which intern with user to show on system
extern u8 UserName_Length;
volatile u8 LDR_LightPrec, LM35_Temp;

LED_config Room_Led_1 = {DIO_PORTC, DIO_PIN5, HIGH};
LED_config Room_Led_2 = {DIO_PORTC, DIO_PIN6, HIGH};
LED_config Room_Led_3 = {DIO_PORTC, DIO_PIN7, HIGH};

// Default flags value
Flags_structConfig Flags = {1, 1, 0, 1, 0, 0, 0};

// Function ProtoType
void Room();
void Room_vFan();
void Room_vSetting();
void Room_Door(void);
void Auto_Fan_Control();
void ROOM_LampOne(void);
void ROOM_LampTwo(void);
void ROOM_LampThree(void);

void main()
{
	// Set Pin Direction
	DIO_enumSetPortDir(DIO_PORTC, DIO_PORT_OUTPUT);
	DIO_enumSetPortDir(DIO_PORTD, 0xFE);
	DIO_enumSetPinDir(DIO_PORTB, DIO_PIN0, DIO_PIN_OUTPUT);
	DIO_enumSetPinDir(DIO_PORTB, DIO_PIN1, DIO_PIN_OUTPUT);

	// Initialize CLCD Pins
	CLCD_vInit();

	ADC_vInit();
	// initialize USART to communicate with laptop with Baud Rate 9600
	USART_vInit();
	// Check EEPROM for password and username and tries left
	EEPROM_vInit();

	DIO_enumSetPinDir(DIO_PORTD, DIO_PIN5, DIO_PIN_OUTPUT);
	TIMER1_vInit();
	// set Timer2 Output PIN
	DIO_enumSetPinDir(DIO_PORTD, DIO_PIN7, DIO_PIN_OUTPUT);
	TIMER2_vInit();

	// Initialize Servo Motor
	SM_vInit();
	SM_vTimer1Degree(90);

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

			CLCD_vClearScreen();
			// print hello message
			CLCD_vSetPosition(2, 7);
			CLCD_vSendString("Welcome ");
			CLCD_vSetPosition(3, ((20 - UserName_Length) / 2) + 1);
			CLCD_vSendString(UserName);
			_delay_ms(1000);
			Flags.OneTimeFlag = 0; // to print it one time which system is open
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
					CLCD_vSendString("Session Time Out");
					Flags.STOP_Flag = 0;
				}
				Flags.OneTimeFlag = 1;
				break;
			}
			Error_Time_Out++;
		}
	} while (KPD_Press != 0X08);
}
//======================================================================================================================================//
void Room_vFan()
{
	CLCD_vSendString("Fan Control : ");
	CLCD_vSendString("1- Fan Off           ");
	CLCD_vSendString("2- Speed 1");
	CLCD_vSendString("3- Speed 2           ");
	CLCD_vSendString("4- Speed 3");
	CLCD_vSendString("5- Speed 4");

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
				TIMER2_vSetCTC(0);
				_delay_ms(100);
				Timer2_vSetPrescaler(TIMER_NO_CLOCK_SOURCE);
				Prescaler_Falg = 0;
				break;
			case '2':
				Error_Time_Out = 0;
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer2_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER2_vSetCTC(63);
				break;
			case '3':
				Error_Time_Out = 0;
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer2_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER2_vSetCTC(126);
				break;
			case '4':
				Error_Time_Out = 0;
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer2_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER2_vSetCTC(189);
				break;
			case '5':
				Error_Time_Out = 0;
				if (Prescaler_Falg == 0) // To Set Prescaler One Time
				{
					Timer2_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
					Prescaler_Falg = 1;
				}
				TIMER2_vSetCTC(255);
				break;
			case 0x08:
				Error_Time_Out = 0;
				CLCD_vSendString("Room Options : ");
				CLCD_vSendString("1- Led1 ON/OFF       ");
				CLCD_vSendString("2- Led2 ON/OFF");
				CLCD_vSendString("3- Led3 ON/OFF       ");
				CLCD_vSendString("4- Room Fan");
				CLCD_vSendString("5- Room Setting");
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
	} while (KPD_Press != 0X08);
}
//======================================================================================================================================//
void Room_vSetting()
{
	USART_u8SendStringSynch("Setting:");
	USART_u8SendData(0X0D);
	USART_u8SendStringSynch("1- Change UserName");
	USART_u8SendData(0X0D);
	USART_u8SendStringSynch("2- Change PassWord");
	USART_u8SendData(0X0D);
	USART_u8SendStringSynch("3- Change UserName & PassWord");
	USART_u8SendData(0X0D);
	USART_u8SendStringSynch("4- Auto Fan Control");
	USART_u8SendData(0X0D);
	do
	{
		Error_State = USART_u8ReceiveData(&KPD_Press);
		if (Error_State == OK)
		{
			switch (KPD_Press)
			{
			case '1':
				USART_u8SendData(0X0D);
				EEPROM_vWrite(EEPROM_UserNameStatus, 0XFF);
				UserName_Set();
				USART_u8SendStringSynch("Setting:");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("1- Change UserName");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("2- Change PassWord");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("2- Change UserName & PassWord");
				USART_u8SendData(0X0D);
				Error_Time_Out = 0;
				break;
			case '2':
				USART_u8SendData(0X0D);
				EEPROM_vWrite(EEPROM_PassWordStatus, 0XFF);
				PassWord_Set();
				USART_u8SendStringSynch("Setting:");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("1- Change UserName");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("2- Change PassWord");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("2- Change UserName & PassWord");
				USART_u8SendData(0X0D);
				Error_Time_Out = 0;
				break;
			case '3':
				USART_u8SendData(0X0D);
				EEPROM_vWrite(EEPROM_UserNameStatus, 0XFF);
				EEPROM_vWrite(EEPROM_PassWordStatus, 0XFF);
				UserName_Set();
				PassWord_Set();
				USART_u8SendStringSynch("Setting:");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("1- Change UserName");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("2- Change PassWord");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("2- Change UserName & PassWord");
				USART_u8SendData(0X0D);
				Error_Time_Out = 0;
				break;
			case '4':
				Auto_Fan_Control();
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
	} while (KPD_Press != 0X08);
}

//======================================================================================================================================//
void Room_Door(void)
{
	USART_u8SendStringSynch("Reception Door : ");
	USART_u8SendData(0X0D);
	USART_u8SendStringSynch("1- Open              ");
	USART_u8SendStringSynch("2- Lock");
	USART_u8SendData(0X0D);
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
	} while (KPD_Press != 0X08);
}
//======================================================================================================================================//
void Auto_Fan_Control()
{
	CLCD_vSendString("Auto Fan Control");
	CLCD_vSendString("1- Open              ");
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
				USART_u8SendData(0X0D);
				if (Flags.STOP_Flag == 1)
				{
					CLCD_vSendString("Session Time Out");
					Flags.STOP_Flag = 0;
				}
				Flags.OneTimeFlag = 1;
				break;
			}
			Error_Time_Out++;
		}
	} while (KPD_Press != 0x08);
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
		}
		else
		{
		}
	} while (KPD_Press != 0X08);
}
