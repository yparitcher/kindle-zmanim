#!/bin/sh
#####
# Copyright (c) 2019 Y Paritcher
#####

CONFDIR=/mnt/us/zman/conf.d
CONFFILE=/mnt/us/zman/zman.conf

CONF="$CONFDIR/$1"

if [ -f $CONF ]; then
	cp $CONF $CONFFILE;
fi
