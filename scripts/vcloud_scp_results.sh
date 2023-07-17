#!/bin/bash
# RES_FILE --> Name of the result file.
#
USERNAME=ubuntu
HOSTS="$1"
count=0
# IDENTITY="~/Desktop/spotless/kdk.pem"
IDENTITY="~/aws.pem"
for HOSTNAME in ${HOSTS}; do
	if [ $count -le 8 -o $count -ge 128 -a $count -le 136 ]; then
		scp -i ${IDENTITY} -o StrictHostKeyChecking=no ${USERNAME}@${HOSTNAME}:resdb/results/${count}.out ./results/
	fi
	count=`expr $count + 1`
done