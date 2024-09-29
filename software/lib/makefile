LIBNAME = libUD5.a

COPT=\
  -DF_CPU=30000000 -DCORE_M0PLUS -D__VTOR_PRESENT -DNOT_USE_CRCTABLE \
  -mcpu=cortex-m0plus -gdwarf-2 -mthumb-interwork -mthumb \
  -Wall -Wno-main -Wshadow -Wcast-align -Wpointer-arith -Wswitch -Wredundant-decls -Wreturn-type -Wshadow -Wunused \
  -fno-builtin -ffunction-sections -fdata-sections -fno-use-cxa-atexit -fomit-frame-pointer \
  -Os

INCLUDEDIRS = -I .\
          -I ./TARGETROOT/LPC84x \
          -I ./TARGETROOT/LPC84x/cmsis \
          -I ./TARGETROOT/LPC84x/lpc_chip_84x/inc \
          -I ./TARGETROOT/LPC84x_EXTRA \
          -I ./TARGETROOT/DXLIB \
          -I ./TARGETROOT/FREERTOS_CM0P/inc

CFLAGS=$(COPT) $(INCLUDEDIRS)

CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar
AS=arm-none-eabi-gcc
ASFLAGS=$(COPT) -c
OBJCOPY=arm-none-eabi-objcopy

ASM_SRC=

THUMB_SRC= \
  ./ud5_exio.cpp \
  ./ud5_gpio.cpp \
  ./ud5_i2c.cpp \
  ./ud5_motor.cpp \
  ./ud5_pcm.cpp \
  ./ud5_pid.cpp \
  ./ud5_rc.cpp \
  ./ud5_spdc.cpp \
  ./ud5_sys.cpp \
  ./ud5_us0.cpp \
  ./ud5_us1.cpp \
  ./ud5_wait.cpp \
  ./ud5_msq.cpp \
  ./ss_oled.cpp


ARM_SRC=

ASM_OBJS = $(ASM_SRC:.s=.o)
THUMB_OBJS = $(THUMB_SRC:.cpp=.o)
ARM_OBJS = $(ARM_SRC:.cpp=.o)

all: lib

libandclean: lib srcclean

lib: $(ASM_OBJS) $(THUMB_OBJS) $(ARM_OBJS)
	$(AR) -rcsv $(LIBNAME) $(ASM_OBJS) $(THUMB_OBJS) $(ARM_OBJS)
	rm -f $(ASM_OBJS) $(ARM_OBJS) $(THUMB_OBJS)

$(ASM_OBJS) : %.o : %.s makefile
	$(CC) -c $(CFLAGS) -x assembler-with-cpp $< -o $@

$(THUMB_OBJS) : %.o : %.cpp makefile
	$(CC) -c $(CFLAGS) -mthumb $< -o $@

$(ARM_OBJS) : %.o : %.cpp makefile
	$(CC) -c $(CFLAGS) $< -o $@

srcclean :
	rm -f $(OBJ)

clean :
	rm -f $(ASM_OBJS) $(THUMB_OBJS) $(ARM_OBJS)-f $(LIBNAME)
