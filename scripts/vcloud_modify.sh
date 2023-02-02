#!/bin/bash
#
USERNAME=ubuntu
HOSTS="$1"
IDENTITY="~/kdkCA.pem"

for HOSTNAME in ${HOSTS}; do
#	ssh -i ${IDENTITY} -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
#  echo '* soft nofile 50000' | sudo tee --append /etc/security/limits.conf
#  "
	ssh -i ${IDENTITY} -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
  sudo apt-get update
  sudo apt-get upgrade
  sudo apt-get install gcc -y
  sudo apt-get install g++ -y
  sudo apt-get install cmake -y
  mkdir resdb
  cd resdb
  mkdir results
  "
#  echo '* hard nofile 90000' | sudo tee --append /etc/security/limits.conf
#  "
done

