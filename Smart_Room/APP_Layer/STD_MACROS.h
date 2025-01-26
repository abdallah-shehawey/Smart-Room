/*
 *<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<    STD_MACROS.h    >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 *  Author : Abdallah Abdelmoemen Shehawey
 *  Layer  : LIB
 *  Description: Standard macros for bit manipulation and register operations
 *              Contains commonly used macros for microcontroller register operations
 */

#ifndef STD_MACROS_h_
#define STD_MACROS_h_

/* Register Configuration */
#define REGISTER_SIZE 8 // Size of register in bits (8-bit microcontroller)

/* Bit Manipulation Macros */
#define SET_BIT(reg, bit) reg |= (1 << bit)          // Set specific bit in register to 1
#define CLR_BIT(reg, bit) reg &= (~(1 << bit))       // Clear specific bit in register to 0
#define TOG_BIT(reg, bit) reg ^= (1 << bit)          // Toggle specific bit in register
#define READ_BIT(reg, bit) (reg & (1 << bit)) >> bit // Read value of specific bit in register

/* Bit Check Macros */
#define IS_BIT_SET(reg, bit) (reg & (1 << bit)) >> bit    // Check if specific bit is set (1)
#define IS_BIT_CLR(reg, bit) !((reg & (1 << bit)) >> bit) // Check if specific bit is clear (0)

/* Register Rotation Macros */
#define ROR(reg, num) reg = (reg << (REGISTER_SIZE - num)) | (reg >> (num)) // Rotate register right
#define ROL(reg, num) reg = (reg >> (REGISTER_SIZE - num)) | (reg << (num)) // Rotate register left

#endif /* STD_MACROS_H_ */
