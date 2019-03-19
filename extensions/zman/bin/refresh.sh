#!/bin/sh
#####
# Copyright (c) 2019 Y Paritcher
#####

CONFDIR=/mnt/us/zman/conf.d
EXTDIR=/mnt/us/extensions/zman

if [ -d $CONFDIR ]; then
	printf "{\n	\"comment001\": \"Zman\",\n	\"comment002\": \"\",\n	\"comment003\": \"\",\n	\"comment004\": \"\",\n	\"items\": [\n	{\n		\"name\": \"Zman\",\n		\"priority\": 0,\n		\"items\": [\n		{\n			\"name\": \"Refresh List\",\n			\"exitmenu\": false,\n			\"date\": false,\n			\"checked\": true,\n			\"refresh\": true,\n			\"action\": \"./bin/refresh.sh\",\n			\"params\": \"refresh\"\n		}" > $EXTDIR/menu.json
	for i in $(ls -1 $CONFDIR); do
		x=$(basename "$i")
		printf ",\n		{\n			\"name\": \"$x\",\n			\"exitmenu\": false,\n			\"date\": false,\n			\"checked\": true,\n			\"refresh\": false,\n			\"action\": \"./bin/zman.sh\",\n			\"params\": \"$x\"\n		}\n" >> $EXTDIR/menu.json;
	done
	printf "		]\n	}\n	]\n}\n" >> $EXTDIR/menu.json
fi

