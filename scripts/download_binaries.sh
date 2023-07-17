#!/bin/bash
#

USERNAME=ubuntu
HOSTS="52.14.129.171"
IDENTITY="~/Desktop/spotless/kdk.pem"

rm -rf ../rundb
rm -rf ../runcl
scp -i ${IDENTITY} ubuntu@${HOSTS}:resdb/rundb ~/Desktop/resdb/resdb
scp -i ${IDENTITY} ubuntu@${HOSTS}:resdb/runcl ~/Desktop/resdb/resdb
