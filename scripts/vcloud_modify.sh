#!/bin/bash
#
USERNAME=ubuntu
HOSTS="$1"
#IDENTITY="~/kdkCA.pem"
IDENTITY="~/kdk_oracle.key"

for HOSTNAME in ${HOSTS}; do
# 	ssh -i ${IDENTITY} -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
#  echo '* soft nofile 50000' | sudo tee --append /etc/security/limits.conf
#  "
	# ssh -i ${IDENTITY} -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
  # rm -rf resdb
  # mkdir resdb
  # cd resdb
  # mkdir results
  # sudo apt-get update
  # sudo apt-get upgrade
  # sudo apt-get install gcc -y
  # sudo apt-get install g++ -y
  # sudo apt-get install cmake -y
  # "
  # sudo iptables-restore < /etc/iptables/rules.v4
  # 
  echo ${HOSTNAME}
  ssh -i ${IDENTITY} -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "
    sudo sed -i 's/MaxStartups 30:50:150/MaxStartups 50:5:150/g' /etc/ssh/sshd_config 
    sudo service sshd restart
  "
  
  # sudo sed -i 's/MaxStartups 10:30:100/MaxStartups 50:5:150/g' /etc/ssh/sshd_config &
  # sudo sed -i 's/#MaxSessions 10/MaxSessions 200/g' /etc/ssh/sshd_config &
  # sudo service sshd restart
#  echo '* hard nofile 90000' | sudo tee --append /etc/security/limits.conf
#  "
done

  # sudo apt-get update
  # sudo apt-get upgrade
  # sudo apt-get install gcc -y
  # sudo apt-get install g++ -y
  # sudo apt-get install cmake -y

  #   sudo sed -i '/-A INPUT -p tcp -m state --state NEW -m tcp --dport 10000:99999 -j ACCEPT/d' /etc/iptables/rules.v4 
  # sudo sed -i 's/:OUTPUT ACCEPT [8:1168]/:OUTPUT ACCEPT [463:49013]/g' /etc/iptables/rules.v4
  # sudo sed -i '/-A INPUT -j REJECT --reject-with icmp-host-prohibited/d' /etc/iptables/rules.v4