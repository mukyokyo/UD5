SHELL   = sh
CC      = arm-none-eabi-gcc
CPP     = arm-none-eabi-g++
OBJCPY  = arm-none-eabi-objcopy
SRCDIR  = .
BINDIR  = ../bin
OBJDIR  = ../obj

CFLAGS  = -DF_CPU=30000000 $(USER_DEFINES) -D__FLASH_TARGET__ -DNDEBUG -D__USE_LPCOPEN -DCORE_M0PLUS -D__VTOR_PRESENT \
          -mcpu=cortex-m0plus -mthumb-interwork -mthumb -nostartfiles \
          -Wall -Wno-main -Wshadow -Wcast-align -Wpointer-arith -Wswitch -Wredundant-decls -Wreturn-type -Wshadow -Wunused \
          -fno-builtin -ffunction-sections -fdata-sections -fno-use-cxa-atexit \
          -Wl,--gc-sections,--allow-multiple-definition,-print-memory-usage --specs=nano.specs --specs=nosys.specs. \
          -T ./TARGETROOT/LPC84x/lpc845_rom_term.x \
          -Os 

INCDIR  = -I ./ \
          -I ../lib \
          -I ./TARGETROOT/LPC84x \
          -I ./TARGETROOT/LPC84x/cmsis \
          -I ./TARGETROOT/LPC84x/lpc_chip_84x/inc \
          -I ./TARGETROOT/LPC84x_EXTRA \
          -I ./TARGETROOT/DXLIB \
          -I ./TARGETROOT/FREERTOS_CM0P/inc


LIBDIR  = -L ../lib \
          -L ./TARGETROOT/LPC84x \
          -L ./TARGETROOT/LPC84x/cmsis \
          -L ./TARGETROOT/LPC84x/lpc_chip_84x \
          -L ./TARGETROOT/LPC84x_EXTRA \
          -L ./TARGETROOT/FREERTOS_CM0P

LIBS    = -lm -lUD5 -lLPC84x_EXTRA -lfreertos_cm0p -llpc84x -llpc_chip_84x -llpc84x

SRCS    = $(wildcard $(SRCDIR)/*.cpp)
TARGETS = $(notdir $(SRCS))

define MAKEALL
$(basename $(1)):$(OBJDIR)/$(basename $(1)).elf
	@if [ ! -d $(BINDIR) ]; then \
		echo ";; mkdir $(BINDIR)"; mkdir $(BINDIR); \
	fi
	$(OBJCPY) -O binary $(OBJDIR)/"$(basename $(1)).elf" "$(BINDIR)/$(basename $(1)).bin"

$(OBJDIR)/$(basename $(1)).elf:$(1)
	@if [ ! -d $(OBJDIR) ]; then \
		echo ";; mkdir $(OBJDIR)"; mkdir $(OBJDIR); \
	fi
ifeq ($(findstring .cpp,"$(1)"),.cpp)
	$(CPP) $(INCDIR) $(CFLAGS) "$(SRCDIR)/$(1)" $(LIBDIR) $(LIBS) -o "$(OBJDIR)/$(basename $(1)).elf"
else
	$(CC) $(INCDIR) $(CFLAGS) "$(SRCDIR)/$(1)" $(LIBDIR) $(LIBS) -o "$(OBJDIR)/$(basename $(1)).elf"
endif
endef

.PHONY: all
all: $(basename $(TARGETS))
$(foreach var,$(TARGETS),$(eval $(call MAKEALL,$(var))))

#make clean
.PHONY: clean
clean: 
	$(RM) -r $(BINDIR) $(OBJDIR)
	$(RM) *.elf *.bin *.bak *.BAK
