#!/bin/bash
# RES_FILE --> Name of the result file.
#
USERNAME=ubuntu
HOSTS="$1"
count=0
# IDENTITY="~/Desktop/pvp/kdk.pem"
#IDENTITY="~/kdkCA.pem"
IDENTITY="~/kdk_oracle.key"

for HOSTNAME in ${HOSTS}; do
	scp -i ${IDENTITY} -o StrictHostKeyChecking=no ${USERNAME}@${HOSTNAME}:resdb/results/${count}.out ./results/
	count=`expr $count + 1`
done