################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../TouchGFX/generated/images/src/BitmapDatabase.cpp \
../TouchGFX/generated/images/src/SVGDatabase.cpp \
../TouchGFX/generated/images/src/image_DCAC.cpp \
../TouchGFX/generated/images/src/image_bg.cpp \
../TouchGFX/generated/images/src/image_image00.cpp \
../TouchGFX/generated/images/src/image_image01.cpp \
../TouchGFX/generated/images/src/image_movie2.cpp \
../TouchGFX/generated/images/src/image_slider_background.cpp \
../TouchGFX/generated/images/src/image_slider_background_filled.cpp \
../TouchGFX/generated/images/src/image_slider_background_vertical.cpp \
../TouchGFX/generated/images/src/image_slider_background_vertical_filled.cpp \
../TouchGFX/generated/images/src/image_slider_knob_circle.cpp \
../TouchGFX/generated/images/src/image_slider_knob_shape.cpp 

OBJS += \
./TouchGFX/generated/images/src/BitmapDatabase.o \
./TouchGFX/generated/images/src/SVGDatabase.o \
./TouchGFX/generated/images/src/image_DCAC.o \
./TouchGFX/generated/images/src/image_bg.o \
./TouchGFX/generated/images/src/image_image00.o \
./TouchGFX/generated/images/src/image_image01.o \
./TouchGFX/generated/images/src/image_movie2.o \
./TouchGFX/generated/images/src/image_slider_background.o \
./TouchGFX/generated/images/src/image_slider_background_filled.o \
./TouchGFX/generated/images/src/image_slider_background_vertical.o \
./TouchGFX/generated/images/src/image_slider_background_vertical_filled.o \
./TouchGFX/generated/images/src/image_slider_knob_circle.o \
./TouchGFX/generated/images/src/image_slider_knob_shape.o 

CPP_DEPS += \
./TouchGFX/generated/images/src/BitmapDatabase.d \
./TouchGFX/generated/images/src/SVGDatabase.d \
./TouchGFX/generated/images/src/image_DCAC.d \
./TouchGFX/generated/images/src/image_bg.d \
./TouchGFX/generated/images/src/image_image00.d \
./TouchGFX/generated/images/src/image_image01.d \
./TouchGFX/generated/images/src/image_movie2.d \
./TouchGFX/generated/images/src/image_slider_background.d \
./TouchGFX/generated/images/src/image_slider_background_filled.d \
./TouchGFX/generated/images/src/image_slider_background_vertical.d \
./TouchGFX/generated/images/src/image_slider_background_vertical_filled.d \
./TouchGFX/generated/images/src/image_slider_knob_circle.d \
./TouchGFX/generated/images/src/image_slider_knob_shape.d 


# Each subdirectory must supply rules for building sources it contributes
TouchGFX/generated/images/src/%.o TouchGFX/generated/images/src/%.su TouchGFX/generated/images/src/%.cyclo: ../TouchGFX/generated/images/src/%.cpp TouchGFX/generated/images/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F412Zx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../TouchGFX/App -I../TouchGFX/target/generated -I../TouchGFX/target -I../Middlewares/ST/touchgfx/framework/include -I../TouchGFX/generated/fonts/include -I../TouchGFX/generated/gui_generated/include -I../TouchGFX/generated/images/include -I../TouchGFX/generated/texts/include -I../TouchGFX/generated/videos/include -I../TouchGFX/gui/include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -femit-class-debug-always -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-TouchGFX-2f-generated-2f-images-2f-src

clean-TouchGFX-2f-generated-2f-images-2f-src:
	-$(RM) ./TouchGFX/generated/images/src/BitmapDatabase.cyclo ./TouchGFX/generated/images/src/BitmapDatabase.d ./TouchGFX/generated/images/src/BitmapDatabase.o ./TouchGFX/generated/images/src/BitmapDatabase.su ./TouchGFX/generated/images/src/SVGDatabase.cyclo ./TouchGFX/generated/images/src/SVGDatabase.d ./TouchGFX/generated/images/src/SVGDatabase.o ./TouchGFX/generated/images/src/SVGDatabase.su ./TouchGFX/generated/images/src/image_DCAC.cyclo ./TouchGFX/generated/images/src/image_DCAC.d ./TouchGFX/generated/images/src/image_DCAC.o ./TouchGFX/generated/images/src/image_DCAC.su ./TouchGFX/generated/images/src/image_bg.cyclo ./TouchGFX/generated/images/src/image_bg.d ./TouchGFX/generated/images/src/image_bg.o ./TouchGFX/generated/images/src/image_bg.su ./TouchGFX/generated/images/src/image_image00.cyclo ./TouchGFX/generated/images/src/image_image00.d ./TouchGFX/generated/images/src/image_image00.o ./TouchGFX/generated/images/src/image_image00.su ./TouchGFX/generated/images/src/image_image01.cyclo ./TouchGFX/generated/images/src/image_image01.d ./TouchGFX/generated/images/src/image_image01.o ./TouchGFX/generated/images/src/image_image01.su ./TouchGFX/generated/images/src/image_movie2.cyclo ./TouchGFX/generated/images/src/image_movie2.d ./TouchGFX/generated/images/src/image_movie2.o ./TouchGFX/generated/images/src/image_movie2.su ./TouchGFX/generated/images/src/image_slider_background.cyclo ./TouchGFX/generated/images/src/image_slider_background.d ./TouchGFX/generated/images/src/image_slider_background.o ./TouchGFX/generated/images/src/image_slider_background.su ./TouchGFX/generated/images/src/image_slider_background_filled.cyclo ./TouchGFX/generated/images/src/image_slider_background_filled.d ./TouchGFX/generated/images/src/image_slider_background_filled.o ./TouchGFX/generated/images/src/image_slider_background_filled.su ./TouchGFX/generated/images/src/image_slider_background_vertical.cyclo ./TouchGFX/generated/images/src/image_slider_background_vertical.d ./TouchGFX/generated/images/src/image_slider_background_vertical.o ./TouchGFX/generated/images/src/image_slider_background_vertical.su ./TouchGFX/generated/images/src/image_slider_background_vertical_filled.cyclo ./TouchGFX/generated/images/src/image_slider_background_vertical_filled.d ./TouchGFX/generated/images/src/image_slider_background_vertical_filled.o ./TouchGFX/generated/images/src/image_slider_background_vertical_filled.su ./TouchGFX/generated/images/src/image_slider_knob_circle.cyclo ./TouchGFX/generated/images/src/image_slider_knob_circle.d ./TouchGFX/generated/images/src/image_slider_knob_circle.o ./TouchGFX/generated/images/src/image_slider_knob_circle.su ./TouchGFX/generated/images/src/image_slider_knob_shape.cyclo ./TouchGFX/generated/images/src/image_slider_knob_shape.d ./TouchGFX/generated/images/src/image_slider_knob_shape.o ./TouchGFX/generated/images/src/image_slider_knob_shape.su

.PHONY: clean-TouchGFX-2f-generated-2f-images-2f-src

