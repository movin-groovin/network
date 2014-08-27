#!/bin/bash



function main {
	tar -zcvf $1 $2 $3
	
	return 0
}



if [ $# -lt 3 ]; then
	echo "Need input parameters. Example: ./make_install binary config output_tar_name"
	exit 1
else
	main $1 $2 $3
	exit 0
fi
