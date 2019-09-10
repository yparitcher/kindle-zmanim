#!/bin/sh
#####
# Copyright (c) 2019 Y Paritcher
#####

CONFDIR=/mnt/us/zman/conf.d
EXTDIR=/mnt/us/extensions/zman
FRAGMENT=/mnt/us/extensions/zman/bin/fragment.json

if [ -d $CONFDIR ]; then
	cp $FRAGMENT $EXTDIR/menu.json
	for i in $(ls -1 $CONFDIR); do
		x=$(basename "$i")
		printf ",\n		{\n			\"name\": \"$x\",\n			\"exitmenu\": false,\n			\"date\": false,\n			\"checked\": true,\n			\"refresh\": false,\n			\"action\": \"./bin/zman.sh\",\n			\"params\": \"$x\"\n		}" >> $EXTDIR/menu.json;
	done
	printf "\n		]\n	}\n	]\n}\n" >> $EXTDIR/menu.json
fi

