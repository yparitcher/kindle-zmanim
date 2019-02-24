#!/bin/sh

#####
# Copyright (c) 2019 Y Paritcher
#####

_FUNCTIONS=/etc/rc.d/functions
[ -f ${_FUNCTIONS} ] && . ${_FUNCTIONS}
WORKDIR=/mnt/us/zman

calculate()
{
	$WORKDIR/program
	msg "zman: new picture" I
}

run()
{
	$WORKDIR/delta
	msg "zman: delta" I
}

# Main
case "${1}" in
	"run" )
		${1}
	;;
	"calculate" )
		${1}
	;;
	* )
		run
	;;
esac

return 0
