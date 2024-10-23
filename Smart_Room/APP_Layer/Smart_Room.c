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
#include "../MCAL_Layer/GIE/GIE_interface.h"
#include "../MCAL_Layer/EEPROM/EEPROM_interface.h"
#include "../MCAL_Layer/SPI/SPI_interface.h"
#include "../MCAL_Layer/USART/USART_interface.h"
#include "../MCAL_Layer/EXTI/EXTI_interface.h"
#include "../MCAL_Layer/TIMER/TIMER_interface.h"

#include "../HAL_Layer/LED/LED_interface.h"
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

LED_config uCCh_2_Led_1  =  {DIO_PORTC, DIO_PIN5, HIGH};
LED_config uCCh_2_Led_2  =  {DIO_PORTC, DIO_PIN6, HIGH};

//Default flags value
Flags_structConfig Flags = {1, 1};

//Function ProtoType
void Room();
void Room_vFan();
void Home_vSetting();

void main()
{
	//Set Pin Direction
	DIO_enumSetPortDir(DIO_PORTA, DIO_PORT_OUTPUT);
	DIO_enumSetPortDir(DIO_PORTC, DIO_PORT_OUTPUT);
	DIO_enumSetPortDir(DIO_PORTD, 0xFE);
	DIO_enumSetPinDir(DIO_PORTB, DIO_PIN0, DIO_PIN_OUTPUT);
	DIO_enumSetPinDir(DIO_PORTB, DIO_PIN1, DIO_PIN_OUTPUT);
	//initialize USART to communicate with laptop with Baud Rate 9600
	USART_vInit();
	//initialize SPI to make 2uC communicate with each other
	SPI_vInit();
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
		}
		//Display Home Screen
		USART_u8SendStringSynch("Select Room :");
		USART_u8SendData(0X0D);//new line 
		USART_u8SendStringSynch("1 - Stair            ");
		USART_u8SendStringSynch("2 - Reception");
		USART_u8SendData(0X0D);//new line
		USART_u8SendStringSynch("3 - Salon            ");
		USART_u8SendStringSynch("4 - Bed Room");
		USART_u8SendData(0X0D);//new line
		USART_u8SendStringSynch("5 - Children Room 1  ");
		USART_u8SendStringSynch("6 - Children Room 2");
		USART_u8SendData(0X0D);//new line
		USART_u8SendStringSynch("7 - Bath Room        ");
		USART_u8SendStringSynch("8 - Kitchen");
		USART_u8SendData(0X0D);//new line
		USART_u8SendStringSynch("9 - Corridor         ");
		USART_u8SendStringSynch("10- Balacon");
		USART_u8SendData(0X0D);//new line 
		USART_u8SendStringSynch("11- Auto Fan Control ");
		USART_u8SendStringSynch("12- Setting");
		USART_u8SendData(0x0D);//new line
		USART_u8SendStringSynch("13- EXIT");
		USART_u8SendData(0x0D);//new line
		do
		{
			//choose number from available choise
			KPD_PressLength = 0, PressVal = 0;
			while (1)
			{
				// get input from Laptop
				Error_State = USART_u8ReceiveData(&KPD_Press);
				//if user press on any button
				if (Error_State == OK && KPD_PressLength == 0)
				{
					//if user press enter
					if (KPD_Press == 0x0D || KPD_Press == 0x0F)
					{
						Error_Time_Out = 0;
					}
					//if user press back space
					else if (KPD_Press == 0x08)
					{
						Error_Time_Out = 0;
					}
					else
					{
						Error_Time_Out = 0;
						KPD_Press -= 48;
						PressVal = PressVal * 10 + KPD_Press;
						KPD_PressLength++;
					}
				}
				else if (Error_State == OK && KPD_PressLength <= 2)
				{
					//if user press enter
					if (KPD_Press == 0x0D || KPD_Press == 0x0F)
					{
						Error_Time_Out = 0;
						break;
					}
					//if user press back space
					else if (KPD_Press == 0x08)
					{
						Error_Time_Out = 0;
						PressVal = PressVal / 10;
						KPD_PressLength--;
					}
					//if user enter valid data
					else
					{
						Error_Time_Out = 0;
						KPD_Press -= 48;
						PressVal = PressVal * 10 + KPD_Press;
						KPD_PressLength++;
					}
				}
				else if (Error_State == OK && KPD_PressLength > 2)
				{
					//if user press enter
					if (KPD_Press == 0x0D || KPD_Press == 0x0F)
					{
						Error_Time_Out = 0;
						break;
					}
					//if user press back space
					else if (KPD_Press == 0x08)
					{
						Error_Time_Out = 0;
						KPD_PressLength--;
					}
					//if user enter valid data
					else
					{
						Error_Time_Out = 0;
						KPD_PressLength++;
					}
				}
				else if (Error_State == TIMEOUT_STATE)
				{
					//when user not press any thing for some time system will close automatic
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
				else
				{

				}
			}
			//if user choose any thing out available choose
			if (PressVal > 13)
			{
				USART_u8SendStringSynch("Invalid Choise");
				USART_u8SendData(0X0D);
			}
		} while (PressVal > 13); //get into infinite loop until user not choose available number

		//check chosen number to open its function
		switch (PressVal)
		{
		case 6 :
			Room();
			break;
		case 13 :
			Flags.OneTimeFlag = 1;
			break;
		default :
			break;
		}
	}
}

//======================================================================================================================================//
void Room()
{
	USART_u8SendStringSynch("Children Room 2 Options : ");
	USART_u8SendData(0X0D);

	USART_u8SendStringSynch("1- Led1 ON/OFF       ");
	USART_u8SendStringSynch("2- Led2 ON/OFF");
	USART_u8SendData(0X0D);
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
				LED_vTog(uCCh_2_Led_1);
				break;
			case '2':
				Error_Time_Out = 0;
				LED_vTog(uCCh_2_Led_2);
				break;
			case '3' :
				Error_Time_Out = 0;
				Child_vFan();
				KPD_Press = NOTPRESSED;
				break;
			case 0x08 :
				Error_Time_Out = 0;
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
	}while (KPD_Press != 0X0D);
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
				USART_u8SendStringSynch("Children Room Options : ");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("1- Led1 ON/OFF       ");
				USART_u8SendStringSynch("2- Led2 ON/OFF");
				USART_u8SendData(0X0D);
				USART_u8SendStringSynch("3- Child Fan");
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
