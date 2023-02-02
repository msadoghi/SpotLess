#!/bin/bash
#
USERNAME=ubuntu
HOSTS="$1"
IDENTITY="~/kdk_oracle.key"

for HOSTNAME in ${HOSTS}; do

	ssh -i ${IDENTITY} -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
  rm -rf resdb
  mkdir resdb
  cd resdb
  mkdir results
  sudo apt-get update
  sudo apt-get upgrade
  sudo apt-get install gcc -y
  sudo apt-get install g++ -y
  sudo apt-get install cmake -y
  "

  sudo sed -i 's/#MaxSessions 10/MaxSessions 200/g' /etc/ssh/sshd_config &
  ssh -i ${IDENTITY} -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
  sudo iptables-restore < /etc/iptables/rules.v4
  "

done