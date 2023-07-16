#!/bin/bash

home_directory="resdb"
nodes=$1
name=$2
result_dir=$3
input="./ifconfig.txt"
i=0
while IFS= read -r line
do
	cmd="scp ubuntu@${line}:${home_directory}/resilientdb/${name}*.out ${result_dir}"
	echo "$cmd"
	$($cmd) &
	i=$(($i+1))
done < "$input"
wait

i=0
while IFS= read -r line
do
	cmd="ssh ubuntu@${line} rm -f ${home_directory}/resilientdb/*;"
	$($cmd) &
	i=$(($i+1))
done < "$input"
wait