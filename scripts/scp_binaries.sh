#!/bin/bash

home_directory="resdb"
nds=4
HOSTS="$1"
ifconfig=1
i=0
#IDENTITY="~/kdkCA.pem"
IDENTITY="~/aws.pem"
for HOSTNAME in ${HOSTS}; do

	if_cmd="scp -i ${IDENTITY} ./ifconfig.txt ubuntu@${HOSTNAME}:${home_directory}/"
	if [ "$i" -lt "$nds" ];then
		cmd="scp -i ${IDENTITY} ./rundb ubuntu@${HOSTNAME}:${home_directory}/"
	else
		cmd="scp -i ${IDENTITY} ./runcl ubuntu@${HOSTNAME}:${home_directory}/"
	fi

	#monitor="scp monitorResults.sh ubuntu@${line}:${home_directory}/resilientdb/"
	
	if [ "$ifconfig" -eq 1 ];then
		echo "$if_cmd"
		$($if_cmd)&
	fi
	#$($monitor) &
	echo "$cmd"
	$($cmd)&
	i=$(($i+1))
done
wait