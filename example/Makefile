# Builds example.nro against the library in the folder above (make it first).
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=/opt/devkitpro")
endif
include $(DEVKITPRO)/libnx/switch_rules

CC := $(DEVKITPRO)/devkitA64/bin/aarch64-none-elf-gcc
TARGET := example
LIBDIR := ..
ARCH   := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS := -O2 -Wall $(ARCH) -D__SWITCH__ -I$(LIBDIR)/include -I$(LIBNX)/include
LDFLAGS := -specs=$(LIBNX)/switch.specs $(ARCH)
LIBS   := -L$(LIBDIR)/lib -L$(LIBNX)/lib -lswitchupdate -lnx -lm

all: $(TARGET).nro

$(TARGET).nro: $(TARGET).elf
	nacptool --create "libswitchupdate example" quatric 1.0.0 $(TARGET).nacp
	elf2nro $< $@ --nacp=$(TARGET).nacp

$(TARGET).elf: main.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.d *.elf *.nro *.nacp
