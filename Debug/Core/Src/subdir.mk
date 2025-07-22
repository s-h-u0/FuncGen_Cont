################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/dds_AD9833.c \
../Core/Src/dipsw_221AMA16R.c \
../Core/Src/dpot_AD5292.c \
../Core/Src/main.c \
../Core/Src/mux_sn74lvc1g3157.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/z_displ_ILI9XXX.c \
../Core/Src/z_touch_XPT2046.c 

C_DEPS += \
./Core/Src/dds_AD9833.d \
./Core/Src/dipsw_221AMA16R.d \
./Core/Src/dpot_AD5292.d \
./Core/Src/main.d \
./Core/Src/mux_sn74lvc1g3157.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/z_displ_ILI9XXX.d \
./Core/Src/z_touch_XPT2046.d 

OBJS += \
./Core/Src/dds_AD9833.o \
./Core/Src/dipsw_221AMA16R.o \
./Core/Src/dpot_AD5292.o \
./Core/Src/main.o \
./Core/Src/mux_sn74lvc1g3157.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/z_displ_ILI9XXX.o \
./Core/Src/z_touch_XPT2046.o 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F412Zx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/dds_AD9833.cyclo ./Core/Src/dds_AD9833.d ./Core/Src/dds_AD9833.o ./Core/Src/dds_AD9833.su ./Core/Src/dipsw_221AMA16R.cyclo ./Core/Src/dipsw_221AMA16R.d ./Core/Src/dipsw_221AMA16R.o ./Core/Src/dipsw_221AMA16R.su ./Core/Src/dpot_AD5292.cyclo ./Core/Src/dpot_AD5292.d ./Core/Src/dpot_AD5292.o ./Core/Src/dpot_AD5292.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/mux_sn74lvc1g3157.cyclo ./Core/Src/mux_sn74lvc1g3157.d ./Core/Src/mux_sn74lvc1g3157.o ./Core/Src/mux_sn74lvc1g3157.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/z_displ_ILI9XXX.cyclo ./Core/Src/z_displ_ILI9XXX.d ./Core/Src/z_displ_ILI9XXX.o ./Core/Src/z_displ_ILI9XXX.su ./Core/Src/z_touch_XPT2046.cyclo ./Core/Src/z_touch_XPT2046.d ./Core/Src/z_touch_XPT2046.o ./Core/Src/z_touch_XPT2046.su

.PHONY: clean-Core-2f-Src

