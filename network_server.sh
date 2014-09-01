#!/bin/bash


CONF_NAME="network_server.conf"
SHADOW_PART="term_advance_daemon"
PATH=/bin/$SHADOW_PART:/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin
if [ "$1" != "D" ]; then
	BINARY=/bin/$SHADOW_PART/network_server
	CONFIG=/etc/$SHADOW_PART/$CONF_NAME
	SHADOW_STRING="--term_advance_daemon"
else
	BINARY=$(pwd)/network_server
	CONFIG=$(pwd)/network_server.conf
	SHADOW_STRING="xxx"
fi
WAIT_SEC=12



function stop {
	local pid=""
	
	for line in $(ps lax | grep -P "$BINARY $CONFIG $SHADOW_STRING"); do
#echo "$line"
		if [ -z $(printf "$line" | grep -o "grep") ]; then
			pid=$(printf "$(printf "$line" | grep -Po "^\d+([ ]|\s)+\d+( |\s)+\d+")" | grep -Po "\d+$")
#echo "Process with pid: $pid"
			kill -s SIGTERM $pid
			
			sleep $WAIT_SEC
			
			kill -s 0 $pid >& "/dev/null"
			[ $? -eq 0 ] && kill -s SIGKILL $pid
			echo "The process has stoped"
			return 0
		fi
		# not that string
	done
	
	return 0
}


function main {
	[ -x $BINARY ] || (echo "File not found: $BINARY or haven't execute permissions"; exit 1)
	if [ $? -eq 1 ]; then
		exit 0
	fi

	case "$1" in
		"start")
			$BINARY $CONFIG $SHADOW_STRING
			;;
		
		"stop")
			stop
			;;
			
		"restart")
			stop
			$BINARY $CONFIG $SHADOW_STRING
			;;
	esac
}


ifs_old=$IFS
IFS=$'\n'
[ -n "$1" ] || (echo "Enter a command: {start, stop, restart}"; exit 0)
main $2
IFS=$ifs_old










