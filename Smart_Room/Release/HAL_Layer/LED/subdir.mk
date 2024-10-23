################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../HAL_Layer/LED/LED_prog.c 

OBJS += \
./HAL_Layer/LED/LED_prog.o 

C_DEPS += \
./HAL_Layer/LED/LED_prog.d 


# Each subdirectory must supply rules for building sources it contributes
HAL_Layer/LED/%.o: ../HAL_Layer/LED/%.c HAL_Layer/LED/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -Os -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega32 -DF_CPU=8000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


