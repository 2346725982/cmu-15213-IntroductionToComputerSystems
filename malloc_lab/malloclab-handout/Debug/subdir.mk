################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../clock.c \
../fcyc.c \
../fsecs.c \
../ftimer.c \
../mdriver.c \
../memlib.c \
../mm-naive.c \
../mm.c 

OBJS += \
./clock.o \
./fcyc.o \
./fsecs.o \
./ftimer.o \
./mdriver.o \
./memlib.o \
./mm-naive.o \
./mm.o 

C_DEPS += \
./clock.d \
./fcyc.d \
./fsecs.d \
./ftimer.d \
./mdriver.d \
./memlib.d \
./mm-naive.d \
./mm.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


