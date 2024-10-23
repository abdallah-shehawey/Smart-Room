/*
 * Security.c
 *
 *  Created on: Aug 31, 2024
 *      Author: Abdallah Abdelmoemen Shehawey
 */

#include <util/delay.h>

#include "STD_TYPES.h"
#include "STD_MACROS.h"

#include "../MCAL_Layer/DIO/DIO_interface.h"
#include "../MCAL_Layer/EEPROM/EEPROM_interface.h"
#include "../MCAL_Layer/USART/USART_interface.h"

#include "Security.h"

/*To get Username And Password Length from number of input */
volatile u8 UserName_Length = 0, PassWord_Length = 0;
/* to Know Number Of Tries */
volatile u8 Tries = Tries_Max ;

/*To get from user and compare it with saved in EEPROM in Check Function */
volatile u8 Check[21]                                       ;
/* to get if User Name or Password is true and false it is default is true */
volatile u8 UserName_Check_Flag = 1, PassWord_Check_Flag = 1;
/* to get bottom from usart and error status */
volatile u8 KPD_Press, Error_State = 1                      ;
volatile u8 UserName[20];

void EEPROM_vInit(void)
{
	/* To get Number of User Name and Password Length from EEPROM for second sign in */
	UserName_Length = EEPROM_vRead(EEPROM_USNL_Location);
	PassWord_Length = EEPROM_vRead(EEPROM_PWL_Location);

	/* To get number of tries left from EEPROM if it lost one of it Maximum */
	if (EEPROM_vRead(EEPROM_NoTries_Location) != NOTPRESSED)
	{
		Tries = EEPROM_vRead(EEPROM_NoTries_Location);
	}
	else
	{
	}
	/* check if There is UserName or Not */
	if (EEPROM_vRead(EEPROM_UserNameStatus) == NOTPRESSED)
	{
		//if No User Name --> Set New User Name
		USART_u8SendStringSynch("Please Sign Up");
		USART_u8SendData(0X0D);
		// Go To function To Set User_Name
		UserName_Set();
	}
	//And also for Password
	if (EEPROM_vRead(EEPROM_PassWordStatus) == NOTPRESSED)
	{
		// Go To function To Set User_Name
		PassWord_Set();
		USART_u8SendStringSynch("Saved Successfully");
		USART_u8SendData(0X0D);
	}
	/*___________________________________________________________________________________________________________________*/

	//check if Tries is lost or not (when user close project and reopen it when he was in time out)
	if (Tries == 0)
	{
		Error_TimeOut();
	}
	else
	{

	}
	u8 i ;
	for(i = 0; i < UserName_Length; i++)
	{
		UserName[i] = EEPROM_vRead(EEPROM_UserNameStartLocation + i);
	}
}

//======================================================================================================================================//

void UserName_Set(void)
{
	USART_u8SendStringSynch("Set UserName");
	USART_u8SendData(0X0D);
	USART_u8SendStringSynch("Maximum char : 20");
	USART_u8SendData(0X0D);

	UserName_Length = 0;
	/*Get username from user*/
	//do while with check to make user enter user name length more than 5 char
	do
	{
		// if username is less than 5 char and User Write any thing
		if (UserName_Length != 0)
		{
			USART_u8SendStringSynch("UserName Must be More than 5 Char");
			USART_u8SendData(0X0D);
			USART_u8SendStringSynch("Re Enter UserName");
			USART_u8SendData(0X0D);
			UserName_Length = 0;
		}
		// get user name from user by using Keypoard
		while (1)
		{
			// get input from Laptop
			Error_State = USART_u8ReceiveData(&KPD_Press);
			// if user name length is valid
			// To prevent user from delete or make any thing else writing if username less is still zero and that mean user didnt write any thing
			if (Error_State == OK && UserName_Length == 0)
			{
				//if user press enter
				if (KPD_Press == 0x0D || KPD_Press == 0x0F)
				{
				}
				//if user press back space
				else if (KPD_Press == 0x08)
				{
				}
				else
				{
					EEPROM_vWrite(EEPROM_UserNameStartLocation + UserName_Length, KPD_Press);
					UserName_Length++;
				}
			}
			//if UserName length is less than 20 user can write and enter and do any thing
			else if (Error_State == OK && UserName_Length < 20)
			{
				//if user press enter
				if (KPD_Press == 0x0D || KPD_Press == 0x0F)
				{
					break;
				}
				//if user press back space
				else if (KPD_Press == 0x08)
				{

					UserName_Length--;
				}
				else
				{
					EEPROM_vWrite(EEPROM_UserNameStartLocation + UserName_Length, KPD_Press);
					UserName_Length++;
				}
			}
			// if user name length is more than 20 do no thing exept enter and delete
			else if (Error_State == OK && UserName_Length >= 20)
			{
				if (KPD_Press == 0x0D || KPD_Press == 0x0F)
				{
					break;
				}
				else if (KPD_Press == 0x08)
				{

					UserName_Length--;
				}
				else
				{
				}
			}
			else
			{
			}
		}
	} while (UserName_Length <= 5); //didn't went out untill user enter more than 5 char
	/*___________________________________________________________________________________________________________________*/

	//To write User Name length in EEPROM to dont lose it in the future and i want it in checking
	EEPROM_vWrite(EEPROM_UserNameStatus, 0x00);
	EEPROM_vWrite(EEPROM_USNL_Location, UserName_Length);
}

//======================================================================================================================================//

void PassWord_Set(void)
{
	//Function to get password from user like UserName Set Function
	USART_u8SendStringSynch("Set PassWord");
	USART_u8SendData(0X0D);
	USART_u8SendStringSynch("Maximum char : 20");
	USART_u8SendData(0X0D);
	PassWord_Length = 0;
	/*Get username from user*/
	do
	{
		// if username is less than 5 char
		if (PassWord_Length != 0)
		{
			USART_u8SendStringSynch("PassWord Must be More than 5 Char");
			USART_u8SendData(0X0D);

			USART_u8SendStringSynch("Re Set PassWord");
			USART_u8SendData(0X0D);
			USART_u8SendStringSynch("Maximum char : 20");
			USART_u8SendData(0X0D);
			PassWord_Length = 0;
		}
		// get user name from user by using Keypoard
		while (1)
		{
			// get input from Laptop
			Error_State = USART_u8ReceiveData(&KPD_Press);
			// if user name length is valid
			// To prevent user from delete or make any thing else writing if username less is still zero and that mean user didnt write any thing
			if (Error_State == OK && PassWord_Length == 0)
			{
				//if user press enter
				if (KPD_Press == 0x0D || KPD_Press == 0x0F)
				{
				}
				//if user press back space
				else if (KPD_Press == 0x08)
				{
				}
				else
				{
					EEPROM_vWrite(EEPROM_PassWordStartLocation + PassWord_Length, KPD_Press);
					PassWord_Length++;
				}
			}
			// if UserName length is less than 20 user can write and enter and do any thing
			else if (Error_State == OK && PassWord_Length < 20)
			{
				//if user press enter
				if (KPD_Press == 0x0D || KPD_Press == 0x0F)
				{
					break;
				}
				//if user press back space
				else if (KPD_Press == 0x08)
				{
					PassWord_Length--;
				}
				//if user enter valid data
				else
				{
					EEPROM_vWrite(EEPROM_PassWordStartLocation + PassWord_Length, KPD_Press);
					PassWord_Length++;
				}
			}
			// if user name length is more than 20 do no thing exept enter and delete
			else if (Error_State == OK && PassWord_Length >= 20)
			{
				// if user press enter
				if (KPD_Press == 0x0D || KPD_Press == 0x0F)
				{
					break;
				}
				// if user press back space
				else if (KPD_Press == 0x08)
				{
					PassWord_Length--;
				}
				//if enter valid do no thing because you skipped valid char you must input
				else
				{
				}
			}
			else
			{
			}
		}
	} while (PassWord_Length <= 5);
	/*___________________________________________________________________________________________________________________*/

	// To write passWord length in EEPROM to dont lose it in the future and i want it in checking
	EEPROM_vWrite(EEPROM_PassWordStatus, 0x00);
	EEPROM_vWrite(EEPROM_PWL_Location, PassWord_Length);
}

//======================================================================================================================================//

//check if user name is true or not
void UserName_Check(void)
{
	USART_u8SendStringSynch("Check UserName");
	USART_u8SendData(0X0D);
	u8 CheckLength = 0;
	UserName_Check_Flag = 1;
	while (1)
	{
		Error_State = USART_u8ReceiveData(&KPD_Press);
		// if user name length is valid
		if (Error_State == OK && CheckLength == 0)
		{
			// if user press enter
			if (KPD_Press == 0x0D || KPD_Press == 0x0F)
			{
			}
			// if user press back space
			else if (KPD_Press == 0x08)
			{
			}
			// if user enter valid data
			else
			{
				Check[CheckLength] = KPD_Press;
				CheckLength++;
			}
		}
		else if (Error_State == OK && CheckLength < 20)
		{
			// if user press enter
			if (KPD_Press == 0x0D || KPD_Press == 0x0F)
			{
				break;
			}
			// if user press back space
			else if (KPD_Press == 0x08)
			{

				CheckLength--;
			}
			// if user enter valid data
			else
			{
				Check[CheckLength] = KPD_Press;
				CheckLength++;
			}
		}
		// if user name length is more than 20 do no thing exept enter and delete
		else if (Error_State == OK && CheckLength >= 20)
		{
			// if user press enter
			if (KPD_Press == 0x0D || KPD_Press == 0x0F)
			{
				break;
			}
			// if user press back space
			else if (KPD_Press == 0x08)
			{

				CheckLength--;
			}
			// if user enter valid data
			else
			{
			}
		}
		else
		{
		}
	}

	/*___________________________________________________________________________________________________________________*/

	// Check if UserName is correct or not
	if (CheckLength == UserName_Length)
	{
		for (u8 i = 0; i <= PassWord_Length; i++)
		{
			if (Check[i] == '\0')
			{
				break;
			}
			else if (Check[i] != EEPROM_vRead(EEPROM_UserNameStartLocation + i))
			{
				UserName_Check_Flag = 0; // if it false make this flag to zero
			}
			else
			{
			}
		}
	}
	else if (CheckLength != UserName_Length)
	{
		UserName_Check_Flag = 0;//and if CheckLength is != username length make it false without checkin because it fale with out checking
	}
	else
	{
	}
}

//======================================================================================================================================//

void PassWord_Check(void)
{

	USART_u8SendStringSynch("Check PassWord");
	USART_u8SendData(0X0D);
	u8 CheckLength = 0;
	PassWord_Check_Flag = 1;

	while (1)
	{
		Error_State = USART_u8ReceiveData(&KPD_Press);
		// if user name length is valid
		if (Error_State == OK && CheckLength == 0)
		{
			// if user press enter
			if (KPD_Press == 0x0D || KPD_Press == 0x0F)
			{
			}
			// if user press back space
			else if (KPD_Press == 0x08)
			{
			}
			// if user enter valid data
			else
			{
				Check[CheckLength] = KPD_Press;
				CheckLength++;
			}
		}
		else if (Error_State == OK && CheckLength < 20)
		{
			if (KPD_Press == 0x0D || KPD_Press == 0x0F)
			{
				break;
			}
			else if (KPD_Press == 0x08)
			{

				CheckLength--;
			}
			else
			{
				Check[CheckLength] = KPD_Press;
				CheckLength++;
			}
		}
		// if user name length is more than 20 do no thing exept enter and delete
		else if (Error_State == OK && CheckLength >= 20)
		{
			if (KPD_Press == 0x0D || KPD_Press == 0x0F)
			{
				break;
			}
			else if (KPD_Press == 0x08)
			{

				CheckLength--;
			}

			else
			{
			}
		}
		else
		{
		}
	}

	/*___________________________________________________________________________________________________________________*/

	// Check if password is correct or not
	if (CheckLength == PassWord_Length)
	{
		for (u8 i = 0; i < PassWord_Length; i++)
		{
			if (Check[i] == '\0')
			{
				break;
			}
			if (Check[i] != EEPROM_vRead(EEPROM_PassWordStartLocation + i))
			{
				PassWord_Check_Flag = 0; //if the pasword is uncorrect make this flag to zero
			}
			else
			{
			}
		}
	}
	else if (CheckLength != PassWord_Length)
	{
		PassWord_Check_Flag = 0; // if CheckLength != PassWord_Length make it false with out checking
	}
	else
	{
	}
}

//======================================================================================================================================//

void Sign_In(void)
{
	while (1)
	{
		/* get username and password from user */
		UserName_Check();
		PassWord_Check();
		//if any flag of them is zero that mean there is one of them is zero
		if (UserName_Check_Flag == 0 || PassWord_Check_Flag == 0)
		{
			//if it false make tries--
			USART_u8SendStringSynch("Invalid Username or Password");
			USART_u8SendData(0X0D);
			Tries--;
			EEPROM_vWrite(EEPROM_NoTries_Location, Tries);

			//if there tries i can use
			if (Tries > 0)
			{
				USART_u8SendStringSynch("Tries Left : ");
				USART_u8SendData(Tries + 48);
				USART_u8SendData(0X0D);
			}
			// if there is no tries any more go to function time out to count few of seconds
			else if (Tries == 0)
			{
				USART_u8SendStringSynch("Tries Left : ");
				USART_u8SendData(Tries + 48);
				USART_u8SendData(0X0D);
				Error_TimeOut();
			}
		}
		//if username and password are correct
		else
		{
			USART_u8SendStringSynch("Successfully Sign in");
			EEPROM_vWrite(EEPROM_NoTries_Location, NOTPRESSED);
			Tries = Tries_Max;
			USART_u8SendData(0X0D);
			break;
		}
	}
}

//======================================================================================================================================//

//to time out error
void Error_TimeOut(void)
{
	USART_u8SendStringSynch("Time out :");

	for (u8 i = 5; i > 0; i--)
	{
		USART_u8SendData(' ');
		//show second left on lcd
		USART_u8SendData(i + 48);
		_delay_ms(1000);
	}
	USART_u8SendData(0X0D);
	//to make tries address Not pressed that mean i dosent spend any tries and make tries == max tries again
	EEPROM_vWrite(EEPROM_NoTries_Location, NOTPRESSED);
	Tries = Tries_Max;
}

//======================================================================================================================================//
