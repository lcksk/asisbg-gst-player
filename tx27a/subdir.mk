################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../autoplugger.c \
../gst-main.c \
../typedetect.c 

OBJS += \
./autoplugger.o \
./gst-main.o \
./typedetect.o 

C_DEPS += \
./autoplugger.d \
./gst-main.d \
./typedetect.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	arm-angstrom-linux-gnueabi-gcc -DMACH_IMX27 -I/home/xpucmo/angstrom/angstrom-dev/staging/armv5te-angstrom-linux-gnueabi/usr/include -I/home/xpucmo/angstrom/angstrom-dev/staging/armv5te-angstrom-linux-gnueabi/usr/include/gstreamer-0.10 -I/home/xpucmo/angstrom/angstrom-dev/staging/armv5te-angstrom-linux-gnueabi/usr/include/glib-2.0 -I/home/xpucmo/angstrom/angstrom-dev/staging/armv5te-angstrom-linux-gnueabi/usr/include/libxml2 -I/home/xpucmo/angstrom/angstrom-dev/staging/armv5te-angstrom-linux-gnueabi/usr/lib/glib-2.0/include -O1 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


