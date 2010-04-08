################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include Release/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: gst-play

# Tool invocations
gst-play: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	arm-angstrom-linux-gnueabi-gcc -L/home/xpucmo/angstrom/angstrom-dev/staging/armv5te-angstrom-linux-gnueabi/usr/lib -L/home/xpucmo/angstrom/angstrom-dev/staging/armv5te-angstrom-linux-gnueabi/usr/lib/gstreamer-0.10 -o"gst-play" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) gst-play
	-@echo ' '

post-build:
	-cp -v gst-play /tftpboot/tx27/usr/local/bin
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
