#!/bin/bash
# RES_FILE --> Name of the result file.
#
USERNAME=ubuntu
IDENTITY="~/aws.pem"
HOSTNAME='150.136.223.66'	# ashburn
count=0
while(( $count<32 ))
do
	scp -i ${IDENTITY} -o StrictHostKeyChecking=no ${USERNAME}@$HOSTNAME:pvp/results/${count}.out ./results/
	count=`expr $count + 1`
done