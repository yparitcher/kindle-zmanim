#####
# Copyright (c) 2018 Y Paritcher
#####

PREFIX=$$HOME/x-tools/arm-unknown-linux-gnueabi/bin/arm-unknown-linux-gnueabi-
CC=gcc
AR=ar
RANLIB=ranlib
CFLAGS=-Wall -Wextra -O2 -std=gnu99 -pedantic $(INC_DIR:%=-I%)

INI_DIR = ini/src
VPATH = src $(INI_DIR)
LIBS=:libzmanim.a fbink m
INC_DIR = libzmanim/include FBInk $(INI_DIR) include
LIBDIR=libzmanim/lib FBInk/Release
LDFLAGS=$(LIBDIR:%=-L%)
LDLIBS=$(LIBS:%=-l%)

.PHONY: libzmanim FBInk submodules clean default kindle
TARGETLIBZMANIM= all
TARGETFBINK= linux

default: submodules kzman

kzman: kzman.o ini.o

submodules: libzmanim FBInk

libzmanim:
	cd $@ && $(MAKE) $(TARGETLIBZMANIM)

FBInk:
	cd $@ && $(MAKE) $(TARGETFBINK)

clean:
	$(MAKE) submodules TARGETLIBZMANIM=cleaner TARGETFBINK=clean
	rm -f kzman kzman.o ini.o

kindle:
	$(MAKE) submodules TARGETLIBZMANIM=kindle TARGETFBINK=legacy CROSS_TC=$$HOME/x-tools/arm-kindle5-linux-gnueabi/bin/arm-kindle5-linux-gnueabi
	$(MAKE) kzman CC=$(PREFIX)gcc AR=$(PREFIX)ar RANLIB=$(PREFIX)ranlib CPPFLAGS+=-DKINDLEBUILD LIBS="$(LIBS) lipc"
	$(PREFIX)strip kzman
	mv -f -t ./zman/ kzman

