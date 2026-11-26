ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif
DEVKITA64 ?= $(DEVKITPRO)/devkitA64

PREFIX  := $(DEVKITA64)/bin/aarch64-none-elf-
CC      := $(PREFIX)gcc
AR      := $(PREFIX)ar
NM      := $(PREFIX)nm
OBJCOPY := $(PREFIX)objcopy
LIBNX   ?= $(DEVKITPRO)/libnx

MBEDTLS := vendor/mbedtls
MINIZ   := vendor/miniz
# Inflate-only miniz with stdio; zlib-style names off so apps that link zlib are unaffected.
MZFLAGS := -DMINIZ_NO_TIME -DMINIZ_NO_DEFLATE_APIS -DMINIZ_NO_ZLIB_APIS \
           -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES
ARCH    := -D__SWITCH__ -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE \
           -ffunction-sections -fdata-sections
CFLAGS  := -O2 -Wall -Wextra $(ARCH) -Iinclude -Iconfig -I$(MBEDTLS)/include -I$(MINIZ) -I$(LIBNX)/include $(MZFLAGS) \
           -DMBEDTLS_CONFIG_FILE=\"mbedtls_nxupdate.h\"
TLSFLAGS := -Os $(ARCH) -Iconfig -I$(MBEDTLS)/include -I$(MBEDTLS)/library -I$(LIBNX)/include \
           -DMBEDTLS_CONFIG_FILE=\"mbedtls_nxupdate.h\"

SRCS     := $(wildcard source/*.c)
OBJS     := $(patsubst source/%.c,build/%.o,$(SRCS))
TLS_SRCS := $(wildcard $(MBEDTLS)/library/*.c)
TLS_OBJS := $(patsubst $(MBEDTLS)/library/%.c,build/mbedtls/%.o,$(TLS_SRCS))
MZ_OBJS  := build/miniz.o
LIB      := lib/libswitchupdate.a

all: $(LIB)

# mbedTLS and miniz are linked into the archive under a private prefix
# (mbedtls_* -> nxupdate_mbedtls_*, mz_* -> nxupdate_mz_*), so apps that
# bring their own copies, built with a different configuration, link against
# both without clashes.
$(LIB): $(OBJS) $(TLS_OBJS) $(MZ_OBJS)
	@mkdir -p lib build/renamed
	@$(NM) $^ | awk '$$NF ~ /^(mbedtls_|psa_|tf_psa_|mz_|tdefl_|tinfl_|miniz_)/ { print $$NF " nxupdate_" $$NF }' | sort -u > build/renamed.syms
	@for o in $^; do $(OBJCOPY) --redefine-syms=build/renamed.syms $$o build/renamed/$$(echo $$o | tr / _); done
	@rm -f $@
	$(AR) rcs $@ build/renamed/*.o

build/%.o: source/%.c include/nxupdate.h source/nxupdate_internal.h source/nxupdate_certs.h config/mbedtls_nxupdate.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/mbedtls/%.o: $(MBEDTLS)/library/%.c config/mbedtls_nxupdate.h
	@mkdir -p build/mbedtls
	@$(CC) $(TLSFLAGS) -c $< -o $@

build/miniz.o: $(MINIZ)/miniz.c $(MINIZ)/miniz.h
	@mkdir -p build
	@$(CC) -O2 $(ARCH) $(MZFLAGS) -I$(MINIZ) -I$(LIBNX)/include -Wno-pragmas -w -c $< -o $@

install: $(LIB)
	@mkdir -p $(DESTDIR)$(DEVKITPRO)/portlibs/switch/lib $(DESTDIR)$(DEVKITPRO)/portlibs/switch/include
	cp $(LIB) $(DESTDIR)$(DEVKITPRO)/portlibs/switch/lib/
	cp include/nxupdate.h $(DESTDIR)$(DEVKITPRO)/portlibs/switch/include/

clean:
	rm -rf build lib

.PHONY: all install clean
