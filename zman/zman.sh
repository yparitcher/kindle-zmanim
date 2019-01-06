#!/bin/sh

_FUNCTIONS=/etc/rc.d/functions
[ -f ${_FUNCTIONS} ] && . ${_FUNCTIONS}
WORKDIR=/mnt/us/zman

calculate()
{
	msg "zman: calculate" I

	if [ "$(date -I)" != "$(cat $WORKDIR/now)" ] || [ -f $WORKDIR/override ] ; then
: '
		if [ -f $WORKDIR/mounted_zman ] ; then	
			umount -l /opt/amazon/screen_saver/adunits/600x800/screensvr.gif
			rm -f $WORKDIR/mounted_zman
			REMOUNT="yes"
		fi
'
		$WORKDIR/program

		msg "zman: new picture" I

		date -I > $WORKDIR/now
: '
		if [ "$REMOUNT" == "yes" ] ; then
			if ! grep "^fsp /opt/amazon/screen_saver/adunits/600x800/screensvr.gif" /proc/mounts > /dev/null 2>&1 ; then
				mount --bind $WORKDIR/done.gif /opt/amazon/screen_saver/adunits/600x800/screensvr.gif
			fi
			touch $WORKDIR/mounted_zman
		fi'
	fi

}

run()
{

	calculate
: '
	if [ "$REMOUNT" != "yes" ] ; then
		if ! grep "^fsp /opt/amazon/screen_saver/adunits/600x800/screensvr.gif" /proc/mounts > /dev/null 2>&1 ; then
			mount --bind $WORKDIR/done.gif /opt/amazon/screen_saver/adunits/600x800/screensvr.gif
		fi
		touch $WORKDIR/mounted_zman
	fi'
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
