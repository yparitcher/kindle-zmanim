#####
# Copyright (c) 2018 Y Paritcher
#####

.PHONY: libzmanim FBInk submodules clean default program kindle
TARGETLIBZMANIM= all
TARGETFBINK= linux

default: program

program: submodules 


submodules: libzmanim FBInk


libzmanim:
	cd $@ && $(MAKE) $(TARGETLIBZMANIM)

FBInk:
	cd $@ && $(MAKE) $(TARGETFBINK)

clean:
	$(MAKE) submodules TARGETLIBZMANIM=cleaner TARGETFBINK=clean

kindle:
	$(MAKE) submodules TARGETLIBZMANIM=kindle TARGETFBINK=legacy

