################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/eeprom.c \
../Src/io_image.c \
../Src/ladder_vm.c \
../Src/main.c \
../Src/oled_display.c \
../Src/scan_engine.c \
../Src/ssd1306.c \
../Src/ssd1306_fonts.c \
../Src/stm32f1xx_hal_msp.c \
../Src/stm32f1xx_it.c \
../Src/syscalls.c \
../Src/sysmem.c \
../Src/system_stm32f1xx.c \
../Src/timer_counter.c \
../Src/uart_console.c 

OBJS += \
./Src/eeprom.o \
./Src/io_image.o \
./Src/ladder_vm.o \
./Src/main.o \
./Src/oled_display.o \
./Src/scan_engine.o \
./Src/ssd1306.o \
./Src/ssd1306_fonts.o \
./Src/stm32f1xx_hal_msp.o \
./Src/stm32f1xx_it.o \
./Src/syscalls.o \
./Src/sysmem.o \
./Src/system_stm32f1xx.o \
./Src/timer_counter.o \
./Src/uart_console.o 

C_DEPS += \
./Src/eeprom.d \
./Src/io_image.d \
./Src/ladder_vm.d \
./Src/main.d \
./Src/oled_display.d \
./Src/scan_engine.d \
./Src/ssd1306.d \
./Src/ssd1306_fonts.d \
./Src/stm32f1xx_hal_msp.d \
./Src/stm32f1xx_it.d \
./Src/syscalls.d \
./Src/sysmem.d \
./Src/system_stm32f1xx.d \
./Src/timer_counter.d \
./Src/uart_console.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/eeprom.cyclo ./Src/eeprom.d ./Src/eeprom.o ./Src/eeprom.su ./Src/io_image.cyclo ./Src/io_image.d ./Src/io_image.o ./Src/io_image.su ./Src/ladder_vm.cyclo ./Src/ladder_vm.d ./Src/ladder_vm.o ./Src/ladder_vm.su ./Src/main.cyclo ./Src/main.d ./Src/main.o ./Src/main.su ./Src/oled_display.cyclo ./Src/oled_display.d ./Src/oled_display.o ./Src/oled_display.su ./Src/scan_engine.cyclo ./Src/scan_engine.d ./Src/scan_engine.o ./Src/scan_engine.su ./Src/ssd1306.cyclo ./Src/ssd1306.d ./Src/ssd1306.o ./Src/ssd1306.su ./Src/ssd1306_fonts.cyclo ./Src/ssd1306_fonts.d ./Src/ssd1306_fonts.o ./Src/ssd1306_fonts.su ./Src/stm32f1xx_hal_msp.cyclo ./Src/stm32f1xx_hal_msp.d ./Src/stm32f1xx_hal_msp.o ./Src/stm32f1xx_hal_msp.su ./Src/stm32f1xx_it.cyclo ./Src/stm32f1xx_it.d ./Src/stm32f1xx_it.o ./Src/stm32f1xx_it.su ./Src/syscalls.cyclo ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.cyclo ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su ./Src/system_stm32f1xx.cyclo ./Src/system_stm32f1xx.d ./Src/system_stm32f1xx.o ./Src/system_stm32f1xx.su ./Src/timer_counter.cyclo ./Src/timer_counter.d ./Src/timer_counter.o ./Src/timer_counter.su ./Src/uart_console.cyclo ./Src/uart_console.d ./Src/uart_console.o ./Src/uart_console.su

.PHONY: clean-Src

