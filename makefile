#####
# Copyright (c) 2018 Y Paritcher
#####

PREFIX=$$HOME/x-tools/arm-unknown-linux-gnueabi/bin/arm-unknown-linux-gnueabi-
PREFIXKT4=arm-kindlepw2-linux-gnueabi-
CC=gcc
AR=gcc-ar
RANLIB=gcc-ranlib
CFLAGS=-Wall -Wextra -O2 -flto $(INC_DIR:%=-I%)

INI_DIR = ini/src
VPATH = src $(INI_DIR)
LIBS=:libzmanim.a fbink m lipc
INC_DIR = libzmanim/include FBInk $(INI_DIR) include
LIBDIR=libzmanim/lib FBInk/Release
LDFLAGS=$(LIBDIR:%=-L%)
LDLIBS=$(LIBS:%=-l%)

.PHONY: libzmanim FBInk submodules clean default kindle
TARGETLIBZMANIM=kindle
TARGETFBINK=kindle

default: KT4

kzman: kzman.o ini.o

submodules: libzmanim FBInk

libzmanim:
	cd $@ && $(MAKE) $(TARGETLIBZMANIM)

FBInk:
	cd $@ && $(MAKE) $(TARGETFBINK) MINIMAL=1 IMAGE=1 OPENTYPE=1 CFLAGS=-flto

clean:
	$(MAKE) submodules TARGETLIBZMANIM=cleaner TARGETFBINK=clean
	rm -f kzman kzman.o ini.o

kindle:
	$(MAKE) submodules TARGETFBINK=legacy CROSS_TC=$$HOME/x-tools/arm-kindle5-linux-gnueabi/bin/arm-kindle5-linux-gnueabi
	$(MAKE) kzman CC=$(PREFIX)gcc AR=$(PREFIX)$(AR) RANLIB=$(PREFIX)$(RANLIB)
	$(PREFIX)strip kzman
	mv -f -t ./zman/ kzman

KT4:
	$(MAKE) libzmanim CC=$(PREFIXKT4)gcc AR=$(PREFIXKT4)$(AR) RANLIB=$(PREFIXKT4)$(RANLIB)
	$(MAKE) FBInk CROSS_TC=$$HOME/x-tools/arm-kindlepw2-linux-gnueabi/bin/arm-kindlepw2-linux-gnueabi
	$(MAKE) kzman CC=$(PREFIXKT4)gcc AR=$(PREFIXKT4)$(AR) RANLIB=$(PREFIXKT4)$(RANLIB)
	$(PREFIXKT4)strip kzman
	cp -t ./zman/ kzman

install:
	rsync kzman pw4:/mnt/us/zman

