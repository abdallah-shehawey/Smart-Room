/*
 * Smart_Home.c
 *
 * Created on: Aug 29, 2024
 * Author: Abdallah Abdelmomen
 *
 * Description: Smart Room Control System
 * This system provides automated control for:
 * - Room lighting with 3 LED zones
 * - Temperature-controlled fan
 * - Door access control
 * - User authentication and settings management
 */

/* ==================== Includes Section Start ==================== */
/* Standard Libraries */
#include <util/delay.h> // For time delay functions
#include "STD_TYPES.h"  // Data type definitions (u8, u16, etc.)
#include "STD_MACROS.h" // Bit manipulation macros (SET_BIT, CLR_BIT, etc.)

/* MCAL (Microcontroller Abstraction Layer) - Hardware Control */
#include "../MCAL_Layer/DIO/DIO_interface.h"       // Digital pins control
#include "../MCAL_Layer/EEPROM/EEPROM_interface.h" // Data storage
#include "../MCAL_Layer/USART/USART_interface.h"   // Serial communication
#include "../MCAL_Layer/TIMER/TIMER_interface.h"   // Timers for PWM and delays
#include "../MCAL_Layer/ADC/ADC_interface.h"       // Analog sensor readings
#include "../MCAL_Layer/GIE/GIE_interface.h"       // Global interrupts
#include "../MCAL_Layer/EXTI/EXTI_interface.h"     // External interrupts

/* HAL (Hardware Abstraction Layer) - Sensors and Actuators */
#include "../HAL_Layer/LED/LED_interface.h"   // LED control functions
#include "../HAL_Layer/LM35/LM35_interface.h" // Temperature sensor
#include "../HAL_Layer/LDR/LDR_interface.h"   // Light sensors
#include "../Hal_Layer/ClCD/CLCD_interface.h" // LCD display
#include "../Hal_Layer/KPD/KPD_interface.h"   // Keypad input

/* Application Layer */
#include "SECURITY/SECURITY_interface.h" // User authentication
/* ==================== Includes Section End ==================== */

/* ==================== System Constants ==================== */
// Timeout duration for user input (in milliseconds)
#define Time_Out 1000UL

// Sensor pin definitions
#define LS_PIN DIO_PIN0 // Light sensor input pin
#define IR_PIN DIO_PIN2 // Motion sensor input pin

/* ==================== System Configuration Structures ==================== */
// System flags for various states and modes
typedef struct
{
  u8 OneTimeFlag : 1;     // Controls welcome screen display (1: show, 0: hide)
  u8 STOP_Flag : 1;       // System timeout indicator (1: active, 0: inactive)
  u8 Auto_Fan : 1;        // Fan control mode (1: automatic, 0: manual)
  u8 Page_One : 1;        // Menu page tracker (1: page 1, 0: page 2)
  u8 Lamp_One : 1;        // LED 1 error state (1: error, 0: normal)
  u8 Lamp_Two : 1;        // LED 2 error state (1: error, 0: normal)
  u8 Lamp_Three : 1;      // LED 3 error state (1: error, 0: normal)
  u8 Temp_Detect : 3;     // Temperature range indicator (0-5)
  u8 Fan_ReturnSpeed : 1; // Fan speed recovery flag (1: recover, 0: normal)
} Flags_structConfig;

/* ==================== Sensor Configurations ==================== */
// Temperature sensor setup - ADC Channel 0
LM35_Config LM35 = {ADC_CHANNEL0, 5, ADC_RES_10_BIT};

// Light sensors setup - ADC Channels 1-3 for different zones
LDR_Config LDR1 = {ADC_CHANNEL1, 5, ADC_RES_10_BIT}; // Zone 1
LDR_Config LDR2 = {ADC_CHANNEL2, 5, ADC_RES_10_BIT}; // Zone 2
LDR_Config LDR3 = {ADC_CHANNEL3, 5, ADC_RES_10_BIT}; // Zone 3

/* ==================== Global Variables ==================== */
// System state variables
volatile u8 Error_State;        // Function return status
volatile u8 KPD_Press;          // Keypad input value
volatile u8 LM35_Degree;        // Current temperature
volatile u8 Error_Time_Out = 0; // Timeout counter
volatile u8 Prescaler_Falg = 0; // Timer prescaler state

// User data
extern u8 UserName[20];    // User's name storage
extern u8 UserName_Length; // Length of username

// Sensor readings
volatile u8 LDR_LightPrec; // Light level percentage
volatile u8 LM35_Temp;     // Temperature value

// Timer variables
volatile u8 Timer_Counter = 0; // General purpose counter
volatile u8 Fan_SaveSpeed = 0; // Stored fan speed level

/* ==================== LED Configurations ==================== */
// Room LED pin assignments and initial states
LED_config Room_Led_1 = {DIO_PORTC, DIO_PIN5, HIGH}; // Main room light
LED_config Room_Led_2 = {DIO_PORTD, DIO_PIN3, HIGH}; // Secondary light
LED_config Room_Led_3 = {DIO_PORTC, DIO_PIN4, HIGH}; // Accent light

// Initial system flags configuration
Flags_structConfig Flags = {
    1, // OneTimeFlag: Show welcome screen
    1, // STOP_Flag: Enable timeout
    0, // Auto_Fan: Manual mode
    1, // Page_One: First menu page
    0, // Lamp_One: No error
    0, // Lamp_Two: No error
    0, // Lamp_Three: No error
    0, // Temp_Detect: No temperature level
    0  // Fan_ReturnSpeed: No speed recovery
};

/* ==================== Function Prototypes ==================== */
/* System_Init: Initializes all hardware components including DIO, LEDs, LCD, ADC,
   USART, EEPROM, timers, and interrupts */
void System_Init(void);

/* Room: Main control interface that displays menu options and handles user input
   for controlling LEDs, fan, door, and settings */
void Room(void);

/* Room_vFan: Controls fan speed with 4 levels and handles both manual and
   automatic fan control modes */
void Room_vFan(void);

/* Room_vSetting: Manages system settings including username/password changes
   and auto fan control configuration */
void Room_vSetting(void);

/* Room_Door: Controls door operations using servo motor, providing open/lock
   functionality */
void Room_Door(void);

/* Auto_Fan_Control: Manages automatic fan control based on temperature readings */
void Auto_Fan_Control(void);

/* ROOM_Lamp: Controls individual room lamps (1-3), handles lamp states and
   error detection */
void ROOM_Lamp(u8 Lamp);

/* ISR_EXTI0_Interrupt: Handles external interrupt for light sensor readings
   and LED control */
void ISR_EXTI0_Interrupt(void);

/* ISR_TIMER2_OVF_MODE: Timer overflow interrupt handler for fan speed and
   motion-based lighting control */
void ISR_TIMER2_OVF_MODE(void);

/* ==================== Main Function ==================== */
/**
 * @brief Main program entry point
 * @details Program flow:
 * 1. Initialize all system components
 * 2. Enter main control loop
 * 3. Check if system needs to show welcome screen
 * 4. Handle user authentication
 * 5. Display welcome message with username
 * 6. Enter room control menu
 */
void main()
{
  System_Init();
  while (1)
  {
    // if System is close and user want to open system
    if (Flags.OneTimeFlag == 1)
    {
      Error_Time_Out = 0;  // Reset timeout counter for new session
      Flags.STOP_Flag = 1; // Enable system timeout monitoring
      CLCD_vSendString("Press enter to open system");
      do
      {
        // Wait for user input through UART
        Error_State = USART_u8ReceiveData(&KPD_Press);
        if (Error_State == OK)
        {
          if (KPD_Press == 0X0D) // 0x0D is ASCII code for Enter key
          {
            break;
          }
        }
      } while (1); // go into infinite loop until press enter

      // Check username and password
      Sign_In();
      // After Successfully sign in
      CLCD_vClearScreen();
      // print hello message
      CLCD_vSetPosition(2, 7); // Position cursor for welcome message
      CLCD_vSendString("Welcome ");
      CLCD_vSetPosition(3, ((20 - UserName_Length) / 2) + 1); // Center username on LCD
      CLCD_vSendString(UserName);
      _delay_ms(1000); // Show welcome message for 1 second

      Flags.OneTimeFlag = 0; // to print it one time when system open
      Room();                // Enter main room control menu
    }
    else
    {
      // Reserved for future system state handling
    }
  }
}

/* System initialization function: Sets up all hardware components and
   configures system parameters */
/**
 * @brief System initialization function
 * @details Initializes:
 * 1. Digital I/O pins for sensors and controls
 * 2. LED configurations for room lighting
 * 3. LCD display for user interface
 * 4. ADC for temperature and light sensors
 * 5. USART for serial communication
 * 6. EEPROM for storing user data
 * 7. Timers for PWM and timing functions
 * 8. Interrupts for sensor monitoring
 * 9. Servo motor for door control
 */
void System_Init(void)
{
  // Configure light sensor input pin
  DIO_enumSetPinDir(DIO_PORTB, DIO_PIN0, DIO_PIN_INPUT);

  // Set Pin Direction
  LED_vInit(Room_Led_1); // Initialize main room LED
  LED_vInit(Room_Led_2); // Initialize secondary LED
  LED_vInit(Room_Led_3); // Initialize accent LED

  // Initialize CLCD Pins
  CLCD_vInit();
  // initialize ADC to Convert From Analog To digital
  ADC_vInit();
  // initialize USART to communicate with laptop with Baud Rate 9600 in config.h
  USART_vInit();
  // Check EEPROM for password and username and tries left
  EEPROM_vInit();

  // Configure PWM output for fan control
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

  // Initialize Servo Motor for door control
  SM_vInit();
  SM_vTimer1Degree(90); // Set door to locked position (90 degrees)

  // SET I-Bit to enable Interrupt
  GIE_vEnable();
  // SET INT2 to execute on change on pin
  EXTI_vEnableInterrupt(EXTI_LINE0);
  EXTI_vSetSignal(EXTI_RISING_EDGE, EXTI_LINE0); // Trigger on rising edge
  // Set Call Back Function for ISR to INT2
  EXTI_vSetCallBack(ISR_EXTI0_Interrupt, EXTI_LINE0);
  // Configure motion sensor input
  DIO_enumSetPinDir(DIO_PORTD, DIO_PIN2, DIO_PIN_INPUT);
}

/* Main room control interface: Handles all room control options and
   user interactions */
/**
 * @brief Room control menu interface
 * @details Provides two menu pages:
 * Page 1:
 * - LED control options (3 zones)
 * - Individual lamp control
 * Page 2:
 * - Fan control
 * - Door control
 * - System settings
 *
 * Features:
 * - Timeout handling for user inactivity
 * - Menu navigation between pages
 * - Direct access to all room controls
 */
void Room()
{
  // Display system controls menu (Fan, Door, Settings)
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
    Flags.Page_One = 1; // Switch to LED control menu next time
  }
  // Display LED control menu
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
    Flags.Page_One = 0; // Switch to system controls menu next time
  }
  else
  {
    // Reserved for additional menu pages
  }

  do
  {
    KPD_Press = 0XFF; // Reset keypad input (0xFF = no key pressed)
    Error_State = USART_u8ReceiveData(&KPD_Press);
    if (Error_State == OK)
    {
      switch (KPD_Press)
      {
      case '1':             // Control LED 1
        Error_Time_Out = 0; // Reset timeout counter
        ROOM_Lamp(1);
        KPD_Press = NOTPRESSED;
        break;
      case '2': // Control LED 2
        Error_Time_Out = 0;
        ROOM_Lamp(2);
        KPD_Press = NOTPRESSED;
        break;
      case '3': // Control LED 3
        Error_Time_Out = 0;
        ROOM_Lamp(3);
        KPD_Press = NOTPRESSED;
        break;
      case '4': // Fan control menu
        Error_Time_Out = 0;
        Room_vFan();
        KPD_Press = NOTPRESSED;
        break;
      case '5': // Door control menu
        Error_Time_Out = 0;
        Room_Door();
        KPD_Press = NOTPRESSED;
        break;
      case '6': // Settings menu
        Error_Time_Out = 0;
        Room_vSetting();
        KPD_Press = NOTPRESSED;
        break;
      case '0': // Toggle between menus
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
      case 0x08: // Back button pressed
        Error_Time_Out = 0;
        CLCD_vClearScreen();
        Flags.OneTimeFlag = 1;
        Flags.Page_One = 1;
        break;
      default:
        break;
      }
    }
    else if (Error_State == TIMEOUT_STATE)
    {
      // Handle system timeout
      if (Error_Time_Out == Time_Out) // Check if maximum timeout reached
      {
        if (Flags.STOP_Flag == 1)
        {
          CLCD_vClearScreen();
          CLCD_vSendString("Session Time Out");
          _delay_ms(1000);
          Flags.STOP_Flag = 0;
          CLCD_vClearScreen();
        }
        else
        {
          // Reserved for additional timeout handling
        }
        Flags.OneTimeFlag = 1;
        break;
      }
      else
      {
        // Reserved for additional timeout states
      };
      Error_Time_Out++; // Increment timeout counter
    }
  } while (KPD_Press != 0X08); // Continue until back button pressed
}

/* Individual lamp control function: Manages lamp states based on user input
   and light sensor readings */
/**
 * @brief Individual lamp control function
 * @param Lamp Lamp number (1-3) to control
 * @details For each lamp:
 * 1. Displays lamp control options (ON/OFF)
 * 2. Monitors light level using LDR sensor
 * 3. Detects lamp errors (if ON but no light detected)
 * 4. Updates LCD with current lamp status
 * 5. Handles timeout conditions
 *
 * Light thresholds:
 * - Below 50%: Considered dark
 * - Above 50%: Considered well-lit
 */
void ROOM_Lamp(u8 Lamp)
{
  if (Lamp == 1) // Control LED 1
  {
    CLCD_vClearScreen();
    CLCD_vSendString("Lamp1 Option :");
    CLCD_vSetPosition(2, 1);
    CLCD_vSendString("1 : ON       2 : OFF");
    KPD_Press = NOTPRESSED;
    do
    {
      // Read current light level from sensor
      LDR_u8GetLightPres(&LDR1, &LDR_LightPrec);
      // Check for lamp error condition (ON command but no light detected)
      if (KPD_Press == '1' && LDR_LightPrec < 50)
      {
        if (Flags.Lamp_One == 0) // No error state currently set
        {
          CLCD_vSetPosition(4, 5);
          _delay_ms(500);
          CLCD_vSendString("  Lamp has Error");
          Flags.Lamp_One = 1; // Set error flag
        }
      }
      if (LDR_LightPrec > 50) // Light detected (lamp is ON)
      {
        Flags.Lamp_One = 0; // Clear error flag
        CLCD_vSetPosition(4, 5);
        CLCD_vSendString("LED Status : ON ");
        Error_State = USART_u8ReceiveData(&KPD_Press);
        if (Error_State == OK)
        {
          switch (KPD_Press)
          {
          case '1': // Already ON - no action needed
            break;
          case '2': // Turn OFF
            LED_vTog(Room_Led_1);
            break;
          case 0X08: // Back to main menu
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
      else if (LDR_LightPrec < 50) // No light detected (lamp is OFF)
      {
        CLCD_vSetPosition(4, 5);
        if (Flags.Lamp_One == 0) // No error condition
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
        // Reserved for other light level conditions
      }
    } while (KPD_Press != 0X08); // Continue until back button pressed
  }
  else if (Lamp == 2)
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
          _delay_ms(500);
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
  else if (Lamp == 3)
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
          _delay_ms(500);
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
}

/* Fan control interface: Manages fan speeds and operation modes */
/**
 * @brief Fan speed control interface
 * @details Provides:
 * 1. Manual speed control:
 *    - OFF
 *    - Speed 1 (25%)
 *    - Speed 2 (50%)
 *    - Speed 3 (75%)
 *    - Speed 4 (100%)
 * 2. Auto fan mode indication
 * 3. Timer prescaler management
 * 4. LCD status updates
 */
void Room_vFan()
{
  CLCD_vClearScreen();
  CLCD_vSendString("Fan Control : ");
  CLCD_vSetPosition(2, 1);
  CLCD_vSendString("1- Off    "); // Fan OFF option
  CLCD_vSendString("2- S 1");     // Speed 1 (25% duty cycle)
  CLCD_vSetPosition(3, 1);
  CLCD_vSendString("3- S 2    "); // Speed 2 (50% duty cycle)
  CLCD_vSendString("4- S 3");     // Speed 3 (75% duty cycle)
  CLCD_vSetPosition(4, 1);
  CLCD_vSendString("5- S 4    "); // Speed 4 (100% duty cycle)

  // Display auto fan status if enabled
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
      case '1': // Turn fan OFF
        Error_Time_Out = 0;
        TIMER0_vSetCTC(0); // Set PWM duty cycle to 0%
        _delay_ms(100);
        Timer0_vSetPrescaler(TIMER_NO_CLOCK_SOURCE); // Stop timer clock
        Prescaler_Falg = 0;
        break;
      case '2': // Set fan to speed 1 (25%)
        Error_Time_Out = 0;
        if (Prescaler_Falg == 0) // Initialize prescaler if not set
        {
          Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
          Prescaler_Falg = 1;
        }
        TIMER0_vSetCTC(63); // ~25% duty cycle
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
        Flags.Page_One = 1;
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

/* Settings management interface: Handles system configuration options */
/**
 * @brief System settings management
 * @details Options:
 * 1. Username management:
 *    - Change username
 *    - Store in EEPROM
 * 2. Password management:
 *    - Change password
 *    - Store in EEPROM
 * 3. Auto fan control:
 *    - Enable/disable
 *    - Temperature-based control
 */
void Room_vSetting()
{
  // Display settings menu options
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
    // Wait for user input
    Error_State = USART_u8ReceiveData(&KPD_Press);
    if (Error_State == OK)
    {
      switch (KPD_Press)
      {
      case '1':                                     // Change username option
        EEPROM_vWrite(EEPROM_UserNameStatus, 0XFF); // Reset username status
        UserName_Set();                             // Call username setup function
        // Redisplay settings menu
        CLCD_vClearScreen();
        CLCD_vSendString("Setting:");
        CLCD_vSetPosition(2, 1);
        CLCD_vSendString("1- Change UserName");
        CLCD_vSetPosition(3, 1);
        CLCD_vSendString("2- Change PassWord");
        CLCD_vSetPosition(4, 1);
        CLCD_vSendString("3- Auto Fan Control");
        Error_Time_Out = 0; // Reset timeout counter
        break;

      case '2':                                     // Change password option
        USART_u8SendData(0X0D);                     // Send carriage return
        EEPROM_vWrite(EEPROM_PassWordStatus, 0XFF); // Reset password status
        PassWord_Set();                             // Call password setup function
        // Redisplay settings menu
        CLCD_vClearScreen();
        CLCD_vSendString("Setting:");
        CLCD_vSetPosition(2, 1);
        CLCD_vSendString("1- Change UserName");
        CLCD_vSetPosition(3, 1);
        CLCD_vSendString("2- Change PassWord");
        CLCD_vSetPosition(4, 1);
        CLCD_vSendString("3- Auto Fan Control");
        Error_Time_Out = 0; // Reset timeout counter
        break;

      case '3': // Auto fan control settings
        Auto_Fan_Control();
        break;

      case 0x08:            // Back button pressed
        Error_Time_Out = 0; // Reset timeout counter
        // Return to main menu
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
    else if (Error_State == TIMEOUT_STATE) // Handle timeout condition
    {
      if (Error_Time_Out == Time_Out) // Maximum timeout reached
      {
        if (Flags.STOP_Flag == 1)
        {
          // Display timeout message
          CLCD_vClearScreen();
          CLCD_vSendString("Session Time Out");
          _delay_ms(1000);
          Flags.STOP_Flag = 0;
          CLCD_vClearScreen();
        }
        Flags.OneTimeFlag = 1; // Set flag to show welcome screen
        break;
      }
      Error_Time_Out++; // Increment timeout counter
    }
  } while (KPD_Press != 0X08); // Continue until back button pressed
}

/* Door control interface: Manages door position using servo motor */
/**
 * @brief Door control using servo motor
 * @details Functions:
 * 1. Door positions:
 *    - Open (0 degrees)
 *    - Locked (90 degrees)
 * 2. User interface:
 *    - Simple open/lock options
 *    - Status display on LCD
 * 3. Safety features:
 *    - Timeout handling
 *    - Position verification
 */
void Room_Door(void)
{
  // Display door control options
  CLCD_vClearScreen();
  CLCD_vSendString("Reception Door : ");
  CLCD_vSetPosition(2, 1);
  CLCD_vSendString("1- Open      ");
  CLCD_vSendString("2- Lock");

  do
  {
    // Wait for user input
    Error_State = USART_u8ReceiveData(&KPD_Press);
    if (Error_State == OK)
    {
      switch (KPD_Press)
      {
      case '1': // Open door (0 degrees)
        SM_vTimer1Degree(0);
        Error_Time_Out = 0;
        break;

      case '2': // Lock door (90 degrees)
        SM_vTimer1Degree(90);
        Error_Time_Out = 0;
        break;

      case 0x08: // Back button pressed
        // Return to main menu
        CLCD_vClearScreen();
        CLCD_vSendString("Room Options : ");
        CLCD_vSetPosition(2, 1);
        CLCD_vSendString("4- Room Fan");
        CLCD_vSetPosition(3, 1);
        CLCD_vSendString("5- Room Door");
        CLCD_vSetPosition(4, 1);
        CLCD_vSendString("6- Room Setting");
        Flags.Page_One = 1;
        break;

      default:
        break;
      }
    }
    else if (Error_State == TIMEOUT_STATE) // Handle timeout condition
    {
      if (Error_Time_Out == Time_Out) // Maximum timeout reached
      {
        if (Flags.STOP_Flag == 1)
        {
          // Display timeout message
          CLCD_vClearScreen();
          CLCD_vSendString("Session Time Out");
          _delay_ms(1000);
          Flags.STOP_Flag = 0;
          CLCD_vClearScreen();
        }
        Flags.OneTimeFlag = 1; // Set flag to show welcome screen
        break;
      }
      Error_Time_Out++; // Increment timeout counter
    }
  } while (KPD_Press != 0X08); // Continue until back button pressed
}

/* Automatic fan control: Handles temperature-based fan speed adjustment */
/**
 * @brief Automatic fan control settings
 * @details Features:
 * 1. Enable/disable auto mode
 * 2. Temperature-based speed control
 * 3. User interface:
 *    - Simple ON/OFF control
 *    - Status display
 * 4. Integration with main fan control
 */
void Auto_Fan_Control()
{
  // Display auto fan control options
  CLCD_vClearScreen();
  CLCD_vSendString("Auto Fan Control");
  CLCD_vSetPosition(2, 1);
  CLCD_vSendString("1- Open     ");
  CLCD_vSendString("2- Close");

  do
  {
    // Wait for user input
    Error_State = USART_u8ReceiveData(&KPD_Press);
    if (Error_State == OK)
    {
      switch (KPD_Press)
      {
      case '1': // Enable auto fan control
        Flags.Auto_Fan = 1;
        Error_Time_Out = 0;
        break;

      case '2': // Disable auto fan control
        Flags.Auto_Fan = 0;
        Error_Time_Out = 0;
        break;

      case 0x08: // Back button pressed
        // Return to settings menu
        USART_u8SendData(0X0D);
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

      default:
        break;
      }
    }
    else if (Error_State == TIMEOUT_STATE) // Handle timeout condition
    {
      if (Error_Time_Out == Time_Out) // Maximum timeout reached
      {
        if (Flags.STOP_Flag == 1)
        {
          // Display timeout message
          CLCD_vClearScreen();
          CLCD_vSendString("Session Time Out");
          _delay_ms(1000);
          Flags.STOP_Flag = 0;
          CLCD_vClearScreen();
        }
        Flags.OneTimeFlag = 1; // Set flag to show welcome screen
        break;
      }
      Error_Time_Out++; // Increment timeout counter
    }
  } while (KPD_Press != 0x08); // Continue until back button pressed
}

/* Fan speed control function: Adjusts fan speed based on temperature or
   manual settings */
/**
 * @brief Temperature-based fan speed control
 * @details Temperature ranges and speeds:
 * - < 20°C: Fan OFF
 * - 20-25°C: Speed 1 (20%)
 * - 26-30°C: Speed 2 (40%)
 * - 31-35°C: Speed 3 (60%)
 * - 36-40°C: Speed 4 (80%)
 * - 41-45°C: Maximum speed (100%)
 *
 * Features:
 * - Automatic mode handling
 * - Manual speed recovery
 * - Prescaler management
 */
void Fan_Speed()
{
  // Read current temperature
  LM35_u8GetTemp(&LM35, &LM35_Degree);

  if (Flags.Auto_Fan == 0) // Manual fan control mode
  {
    if (Flags.Fan_ReturnSpeed == 1) // Need to restore previous speed
    {
      Flags.Temp_Detect = 0;
      switch (Fan_SaveSpeed)
      {
      case 0: // Fan OFF
        TIMER0_vSetCTC(0);
        Fan_SaveSpeed = 0;
        break;
      case 1: // Speed 1 (25%)
        TIMER0_vSetCTC(63);
        Fan_SaveSpeed = 0;
        break;
      case 2: // Speed 2 (50%)
        TIMER0_vSetCTC(126);
        Fan_SaveSpeed = 0;
        break;
      case 3: // Speed 3 (75%)
        TIMER0_vSetCTC(189);
        Fan_SaveSpeed = 0;
        break;
      case 4: // Speed 4 (100%)
        TIMER0_vSetCTC(255);
        Fan_SaveSpeed = 0;
        break;
      default:
        break;
      }
      Flags.Fan_ReturnSpeed = 0;
    }
  }
  else if (Flags.Auto_Fan == 1) // Automatic temperature-based control
  {
    // Temperature ranges and corresponding fan speeds
    if (LM35_Degree < 20) // Below 20°C - turn off fan
    {
      Flags.Temp_Detect = 0;
      TIMER0_vSetCTC(0);
      _delay_ms(100);
      Timer0_vSetPrescaler(TIMER_NO_CLOCK_SOURCE);
      Flags.Fan_ReturnSpeed = 1;
    }
    else if (LM35_Degree >= 20 && LM35_Degree <= 25) // 20-25°C - Speed 1
    {
      if (Flags.Temp_Detect != 1) // Only change speed if needed
      {
        if (Prescaler_Falg == 0)
        {
          Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
          Prescaler_Falg = 1;
        }
        TIMER0_vSetCTC(50); // ~20% speed
        Flags.Temp_Detect = 1;
        Flags.Fan_ReturnSpeed = 1;
      }
    }
    else if (LM35_Degree >= 26 && LM35_Degree <= 30) // 26-30°C - Speed 2
    {
      if (Flags.Temp_Detect != 2)
      {
        if (Prescaler_Falg == 0) // To Set Prescaler One Time
        {
          Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
          Prescaler_Falg = 1;
        }
        TIMER0_vSetCTC(100); // ~40% speed
        Flags.Temp_Detect = 2;
        Flags.Fan_ReturnSpeed = 1;
      }
    }
    else if (LM35_Degree >= 31 && LM35_Degree <= 35) // 31-35°C - Speed 3
    {
      if (Flags.Temp_Detect != 3)
      {
        if (Prescaler_Falg == 0) // To Set Prescaler One Time
        {
          Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
          Prescaler_Falg = 1;
        }
        TIMER0_vSetCTC(150); // ~60% speed
        Flags.Temp_Detect = 3;
        Flags.Fan_ReturnSpeed = 1;
      }
    }
    else if (LM35_Degree >= 36 && LM35_Degree <= 40) // 36-40°C - Speed 4
    {
      if (Flags.Temp_Detect != 4)
      {
        if (Prescaler_Falg == 0) // To Set Prescaler One Time
        {
          Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
          Prescaler_Falg = 1;
        }
        TIMER0_vSetCTC(200); // ~80% speed
        Flags.Temp_Detect = 4;
        Flags.Fan_ReturnSpeed = 1;
      }
    }
    else if (LM35_Degree >= 41 && LM35_Degree <= 45) // 41-45°C - Maximum speed
    {
      if (Flags.Temp_Detect != 5)
      {
        if (Prescaler_Falg == 0) // To Set Prescaler One Time
        {
          Timer0_vSetPrescaler(TIMER_DIVISION_FACTOR_256);
          Prescaler_Falg = 1;
        }
        TIMER0_vSetCTC(255); // 100% speed
        Flags.Temp_Detect = 5;
        Flags.Fan_ReturnSpeed = 1;
      }
    }
    else
    {
      // Reserved for undefined temperature ranges
    }
  }
}

/* External interrupt handler: Processes light sensor inputs and controls
   LED states */
/**
 * @brief External interrupt handler for light sensor
 * @details Operations:
 * 1. Reads light sensor status
 * 2. Controls LEDs based on light levels:
 *    - Checks each zone separately
 *    - Toggles LEDs in dark conditions
 * 3. Implements automatic light control
 * 4. Handles sensor thresholds
 */
void ISR_EXTI0_Interrupt(void)
{
  u8 LS_Status = 0;
  // Read light sensor pin state
  DIO_enumReadPinVal(DIO_PORTB, LS_PIN, &LS_Status);

  if (LS_Status == 1) // High light level detected
  {
    // Reserved for bright condition handling
  }
  else if (LS_Status == 0) // Low light level detected
  {
    // Control LED 1 based on light level
    LDR_LightPrec = 0XFF; // Reset light reading
    LDR_u8GetLightPres(&LDR1, &LDR_LightPrec);
    if (LDR_LightPrec < 50) // Dark condition detected
    {
      LED_vTog(Room_Led_1); // Toggle LED 1
    }
    else
    {
      // Reserved for bright condition
    }

    // Control LED 2 based on light level
    LDR_LightPrec = 0XFF; // Reset light reading
    LDR_u8GetLightPres(&LDR2, &LDR_LightPrec);
    if (LDR_LightPrec < 50) // Dark condition detected
    {
      LED_vTog(Room_Led_2); // Toggle LED 2
    }
    else
    {
      // Reserved for bright condition
    }

    // Control LED 3 based on light level
    LDR_LightPrec = 0XFF; // Reset light reading
    LDR_u8GetLightPres(&LDR3, &LDR_LightPrec);
    if (LDR_LightPrec < 50) // Dark condition detected
    {
      LED_vTog(Room_Led_3); // Toggle LED 3
    }
    else
    {
      // Reserved for bright condition
    }
  }
  else
  {
    // Reserved for undefined sensor states
  }
}

/* Timer overflow interrupt handler: Manages timing-based operations and
   sensor readings */
/**
 * @brief Timer overflow interrupt handler
 * @details Functions:
 * 1. Manages system timing
 * 2. Controls fan speed updates
 * 3. Handles motion detection:
 *    - Reads IR sensor
 *    - Controls lighting based on motion
 * 4. Monitors light levels
 * 5. Updates LED states
 */
void ISR_TIMER2_OVF_MODE()
{
  Timer_Counter++; // Increment system timer
  u8 IR_PinRead = 0;
  Fan_Speed(); // Update fan speed based on temperature

  // Read motion sensor state
  DIO_enumReadPinVal(DIO_PORTD, IR_PIN, &IR_PinRead);

  if (IR_PinRead == 1) // Motion detected
  {
    Timer_Counter = 0; // Reset inactivity timer
  }
  else if (IR_PinRead == 0 && Timer_Counter == 3) // No motion for 3 cycles
  {
    // Check and control LED 1
    LDR_LightPrec = 0XFF; // Reset light reading
    LDR_u8GetLightPres(&LDR1, &LDR_LightPrec);
    if (LDR_LightPrec > 50) // Room is well lit
    {
      LED_vTog(Room_Led_1); // Toggle LED off
      Timer_Counter = 0;    // Reset timer after action
    }
    else
    {
      // Reserved for dark condition
    }

    // Check and control LED 2
    LDR_LightPrec = 0XFF; // Reset light reading
    LDR_u8GetLightPres(&LDR2, &LDR_LightPrec);
    if (LDR_LightPrec > 50) // Room is well lit
    {
      LED_vTog(Room_Led_2); // Toggle LED off
      Timer_Counter = 0;    // Reset timer after action
    }
    else
    {
      // Reserved for dark condition
    }

    // Check and control LED 3
    LDR_LightPrec = 0XFF; // Reset light reading
    LDR_u8GetLightPres(&LDR3, &LDR_LightPrec);
    if (LDR_LightPrec > 50) // Room is well lit
    {
      LED_vTog(Room_Led_3); // Toggle LED off
      Timer_Counter = 0;    // Reset timer after action
    }
    else
    {
      // Reserved for dark condition
    }
  }
}
