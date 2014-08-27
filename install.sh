#!/bin/bash


INSTALL_PATH_BIN="/bin"
INSTALL_PATH_CONF="/etc"



function main {
	# binary, config
	local namesArr[2]
	i=0
	
	
	for line in $(tar -tf $1); do
		namesArr[$i]=$line
		i+=1
	done
	#echo -e "\n\n${namesArr[0]} === ${namesArr[1]}"
	if [ $? -ne 0 ]; then
		echo "Error of archive lookup"
		return 1
	fi
	tar -xvzf $1 > /dev/null
	
	chown root:root ${namesArr[0]} ${namesArr[1]}
	chmod a+x ${namesArr[0]}
	chmod u+s ${namesArr[0]}
	
	mv ${namesArr[0]} $INSTALL_PATH_BIN/${namesArr[0]}
	mv ${namesArr[1]} $INSTALL_PATH_CONF/${namesArr[1]}
	
	# to run the server
	$INSTALL_PATH_BIN/${namesArr[0]} $INSTALL_PATH_CONF/${namesArr[1]} $2
	
	
	return 0
}



if [ $# -lt 2 ]; then
	echo "Enter an archive name and special magic string, "\
		 "taht will be a cmdline argument of the server"
	exit 1
fi
if [ $(whoami) != "root" ]; then
	echo "This script need run under roor account"
	exit 2
fi

main $1 $2
exit $?




