#!/bin/bash

######
# This script allows to compile and run the code. You need to specify IP addresses of your servers and clients. The scripts expect three arguments and the result is stored in a folder named "results". Do create a folder with the name "results" before running this script.
######

i=$1   # Argument 1 to script --> Number of replicas
cli=$2 # Argument 2 to script --> Number of clients
name=$3
runs=$4
bsize=$5 # Argumnet 3 to script --> Batch Size
if [ -z $bsize ]; then
	bsize=100
fi

SNODES=(
	# r0
	"172.31.2.157"
	"172.31.15.161"
	"172.31.8.218"
	"172.31.1.45"
	# "10.138.0.3"
	# "10.138.0.7"
	# "10.138.0.8"
	# "10.138.0.9"
	# "10.138.0.10"
	# "10.138.0.11"
	# "10.138.0.12"
	# "10.138.0.13"
	# "10.138.0.14"
	# "10.138.0.15"
	# "10.138.0.16"
	# "10.138.0.17"
	# r1
	# "10.128.0.46"
	# "10.128.0.40"
	# "10.128.0.55"
	# "10.128.0.52"
	# "10.128.0.62"
	# "10.128.0.133"
	# "10.128.0.59"
	# "10.128.0.32"
	# "10.128.0.67"
	# "10.128.0.70"
	# "10.128.0.48"
	# "10.128.0.61"
	# "10.128.0.42"
	# "10.128.0.78"
	# "10.128.0.108"
	# "10.128.0.63"
	# r2
	# "10.162.0.16"
	# "10.162.0.13"
	# "10.162.0.4"
	# "10.162.0.8"
	# "10.162.0.2"
	# "10.162.0.12"
	# "10.162.0.7"
	# "10.162.0.6"
	# "10.162.0.11"
	# "10.162.0.9"
	# "10.162.0.17"
	# "10.162.0.10"
	# "10.162.0.3"
	# "10.162.0.15"
	# "10.162.0.14"
	# "10.162.0.5"
	# # r3
	# "10.164.0.13"
	# "10.164.0.14"
	# "10.164.0.2"
	# "10.164.0.6"
	# "10.164.0.7"
	# "10.164.0.29"
	# "10.164.0.19"
	# "10.164.0.5"
	# "10.164.0.30"
	# "10.164.0.27"
	# "10.164.0.16"
	# "10.164.0.21"
	# "10.164.0.9"
	# "10.164.0.20"
	# "10.164.0.22"
	# "10.164.0.28"
)

CNODES=(
	#r0
	"172.31.0.114"
	# "10.138.0.21"
	# "10.138.0.19"
	# "10.138.0.20"
	# "10.138.0.18"
	# #r1
	# "10.128.0.135"
	# "10.128.0.134"
	# "10.128.0.136"
	# "10.128.0.137"
	# #r2
	# "10.162.0.19"
	# "10.162.0.20"
	# "10.162.0.18"
	# "10.162.0.21"
	# #r3
	# "10.164.0.32"
	# "10.164.0.36"
	# "10.164.0.34"
	# "10.164.0.33"

)
rm ifconfig.txt hostnames.py

# Building file ifconfig.txt
#
count=0
while (($count < $i)); do
	echo ${SNODES[$count]} >>ifconfig.txt
	count=$((count + 1))
done

count=0
while (($count < $cli)); do
	echo ${CNODES[$count]} >>ifconfig.txt
	count=$((count + 1))
done

# Building file hostnames
#
echo "hostip = [" >>hostnames.py
count=0
while (($count < $i)); do
	echo -e "\""${SNODES[$count]}"\"," >>hostnames.py
	count=$((count + 1))
done

count=0
while (($count < $cli)); do
	echo -e "\""${CNODES[$count]}"\"," >>hostnames.py
	count=$((count + 1))
done
echo "]" >>hostnames.py

echo "hostmach = [" >>hostnames.py
count=0
while (($count < $i)); do
	echo "\""${SNODES[$count]}"\"," >>hostnames.py
	count=$((count + 1))
done

count=0
while (($count < $cli)); do
	echo "\""${CNODES[$count]}"\"," >>hostnames.py
	count=$((count + 1))
done
echo "]" >>hostnames.py

# Compiling the Code
# make clean; make -j8

tm=0

# Copy to scripts
cp run* scripts/
cp ifconfig.txt scripts/
cp config.h scripts/
cp hostnames.py scripts/
cd scripts

# Number of times you want to run the code (default 1)
while [ $tm -lt $runs ]; do
	python3 simRun.py $i s${i}_c${cli}_results_${name}_b${bsize}_run${tm}_node $tm

	tm=$((tm + 1))
done

# Go back
cd ..
