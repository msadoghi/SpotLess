#!/bin/bash
#

USERNAME=ubuntu
HOSTS="$1"
IDENTITY="~/aws.pem"
nodes=3
i=0
for HOSTNAME in ${HOSTS}; do
  if [ "$i" -lt "$nodes" ];then
    echo ${HOSTNAME}
    scp -i ${IDENTITY} -o StrictHostKeyChecking=no ../rundb ${USERNAME}@${HOSTNAME}:
    ssh -i ${IDENTITY} -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
    cd resdb
    rm -rf rundb
    rm -rf results
    mkdir results
    cd ..
    cp ./rundb ./resdb
    rm -rf rundb
    "
  else
    echo ${HOSTNAME}
    scp -i ${IDENTITY} -o StrictHostKeyChecking=no ../runcl ${USERNAME}@${HOSTNAME}:
    ssh -i ${IDENTITY} -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
    cd resdb
    rm -rf runcl
    rm -rf results
    mkdir results
    cd ..
    cp ./runcl ./resdb
    rm -rf runcl
    "
  fi
  i=$(($i+1))
#  echo '* hard nofile 90000' | sudo tee --append /etc/security/limits.conf
#  "
done
