#!/bin/sh

_FUNCTIONS=/etc/rc.d/functions
[ -f ${_FUNCTIONS} ] && . ${_FUNCTIONS}
WORKDIR=/mnt/us/zman

calculate()
{
	msg "zman: calculate" I
	$WORKDIR/program
	msg "zman: new picture" I
}

run()
{
	calculate
	DELTA=$($WORKDIR/delta)
	lipc-set-prop -i  com.lab126.powerd rtcWakeup $DELTA
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
