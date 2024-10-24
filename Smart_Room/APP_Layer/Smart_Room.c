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

#include "../HAL_Layer/LED/LED_interface.h"
#include "../HAL_Layer/LM35/LM35_interface.h"
#include "Security.h"

//Used Flags
typedef struct
{
	unsigned char OneTimeFlag : 1;
	unsigned char STOP_Flag : 1;
}Flags_structConfig;

#define Time_Out               1000UL  //Maximum Time Allow when not press any thing

volatile u8 Error_State, KPD_Press, SPI_Recieve;
volatile u8 KPD_PressLength = 0,  PressVal = 0;
volatile u8 Error_Time_Out = 0;  // To count time out allow for user
extern  u8 UserName[20];          // extern user name which intern with user to show on system

LED_config Room_Led_1  =  {DIO_PORTC, DIO_PIN5, HIGH};
LED_config Room_Led_2  =  {DIO_PORTC, DIO_PIN6, HIGH};
LED_config Room_Led_3  =  {DIO_PORTC, DIO_PIN7, HIGH};

//Default flags value
Flags_structConfig Flags = {1, 1};

//Function ProtoType
void Room();
void Room_vFan();
void Home_vSetting();

void main()
{
	//Set Pin Direction
	DIO_enumSetPortDir(DIO_PORTC, DIO_PORT_OUTPUT);
	DIO_enumSetPortDir(DIO_PORTD, 0xFE);
	DIO_enumSetPinDir(DIO_PORTB, DIO_PIN0, DIO_PIN_OUTPUT);
	DIO_enumSetPinDir(DIO_PORTB, DIO_PIN1, DIO_PIN_OUTPUT);

	ADC_vInit();
	//initialize USART to communicate with laptop with Baud Rate 9600
	USART_vInit();
	//Check EEPROM for password and username and tries left
	EEPROM_vInit();
	//set Timer0 Output PIN
	DIO_enumSetPinDir(DIO_PORTB, DIO_PIN3, DIO_PIN_OUTPUT);
	TIMER0_vInit();
	//set Timer2 Output PIN
	DIO_enumSetPinDir(DIO_PORTD, DIO_PIN7, DIO_PIN_OUTPUT);
	TIMER2_vInit();

	while (1)
	{
		//if System is close and user want to open system
		if (Flags.OneTimeFlag == 1)
		{
			Error_Time_Out = 0;
			Flags.STOP_Flag = 1;
			USART_u8SendStringSynch("Press enter to open system");
			USART_u8SendData(0X0D);
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
			}while (1); //go into infinite loop until press enter
			//Check username and password
			Sign_In();
			//print hello message
			USART_u8SendStringSynch("Welcome ");
			USART_u8SendStringSynch(UserName);
			USART_u8SendData(0X0D); // make new line
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
	USART_u8SendStringSynch("Room Options : ");
	USART_u8SendData(0X0D);

	USART_u8SendStringSynch("1- Led1 ON/OFF       ");
	USART_u8SendStringSynch("2- Led2 ON/OFF");
	USART_u8SendData(0X0D);
	USART_u8SendStringSynch("1- Led3 ON/OFF       ");
	USART_u8SendStringSynch("3- Room Fan");
	USART_u8SendData(0X0D);

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
				LED_vTog(Room_Led_1);
				break;
			case '2':
				Error_Time_Out = 0;
				LED_vTog(Room_Led_2);
				break;
			case '3':
				Error_Time_Out = 0;
				LED_vTog(Room_Led_3);
				break;
			case '4' :
				Error_Time_Out = 0;
				Room_vFan();
				KPD_Press = NOTPRESSED;
				break;
			case 0x08 :
				Error_Time_Out = 0;
				USART_u8SendData(0X0D);
				Flags.OneTimeFlag = 1; // to print it one time which system is open
				break;
			default :
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
			Error_Time_Out ++;
		}
	}while (KPD_Press != 0X08);
}
//======================================================================================================================================//
void Room_vFan()
{
	USART_u8SendStringSynch("Fan Control : ");
	USART_u8SendData(0X0D);
	USART_u8SendStringSynch("1- Fan Off           ");
	USART_u8SendStringSynch("2- Speed 1");
	USART_u8SendData(0X0D);
	USART_u8SendStringSynch("3- Speed 2           ");
	USART_u8SendStringSynch("4- Speed 3");
	USART_u8SendData(0X0D);
	USART_u8SendStringSynch("5- Speed 4");
	USART_u8SendData(0X0D);

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
				break;
			case '2':
				Error_Time_Out = 0;
				TIMER2_vSetCTC(63);
				break;
			case '3':
				Error_Time_Out = 0;
				TIMER2_vSetCTC(126);
				break;
			case '4':
				Error_Time_Out = 0;
				TIMER2_vSetCTC(189);
				break;
			case '5':
				Error_Time_Out = 0;
				TIMER2_vSetCTC(255);
				break;
			case 0x08 :
				Error_Time_Out = 0;
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("Room Option : ");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("1- Led1 ON/OFF       ");
				USART_u8SendStringSynch("2- Led2 ON/OFF");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("3- Room Fan");
				USART_u8SendData(0X0D);
				break;
			default :
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
			Error_Time_Out ++;
		}
	}while (KPD_Press != 0X08);
}
