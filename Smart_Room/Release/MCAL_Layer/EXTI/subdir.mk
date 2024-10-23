################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../MCAL_Layer/EXTI/EXTI_prog.c 

OBJS += \
./MCAL_Layer/EXTI/EXTI_prog.o 

C_DEPS += \
./MCAL_Layer/EXTI/EXTI_prog.d 


# Each subdirectory must supply rules for building sources it contributes
MCAL_Layer/EXTI/%.o: ../MCAL_Layer/EXTI/%.c MCAL_Layer/EXTI/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -Os -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega32 -DF_CPU=8000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


