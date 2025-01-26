/*
 *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<    STD_TYPES.h    >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 *  Author : Abdallah Abdelmoemen Shehawey
 *  Layer  : LIB
 *  Description: Standard types and definitions header file
 *              Contains type definitions and common macros used throughout the project
 */

#ifndef STD_TYPES_H_
#define STD_TYPES_H_

/* Standard Integer Types */
typedef unsigned char u8;           // 8-bit unsigned integer (0 to 255)
typedef signed char s8;             // 8-bit signed integer (-128 to +127)
typedef unsigned short int u16;     // 16-bit unsigned integer (0 to 65,535)
typedef signed short int s16;       // 16-bit signed integer (-32,768 to +32,767)
typedef unsigned long int u32;      // 32-bit unsigned integer (0 to 4,294,967,295)
typedef signed long int s32;        // 32-bit signed integer (-2,147,483,648 to +2,147,483,647)
typedef unsigned long long int u64; // 64-bit unsigned integer
typedef signed long long int s64;   // 64-bit signed integer
typedef float f32;                  // 32-bit floating point
typedef double f64;                 // 64-bit floating point

/* Common Definitions */
#define NULL 0 // Null pointer definition

/* Enable/Disable States */
#define ENABLE 1  // Enable state for features/peripherals
#define DISABLE 0 // Disable state for features/peripherals

/* Function States */
#define IDLE 0 // Function/peripheral is in idle state
#define BUSY 1 // Function/peripheral is busy

/* Return States */
#define OK 0            // Operation completed successfully
#define NOK 1           // Operation failed
#define NULL_POINTER 2  // Null pointer error
#define BUSY_STATE 3    // Resource is busy
#define TIMEOUT_STATE 4 // Operation timed out

#endif /* STD_TYPES_H_ */
