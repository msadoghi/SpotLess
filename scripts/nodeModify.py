#!/usr/bin/python
#
import os,sys,datetime,re
import shlex
import subprocess
import itertools
from sys import argv
from hostnames import *

#identity="expodb_key.pem"
machines = hostip_machines
#mach=hostmach

# running the experiment
cmd = './vcloud_modify.sh \"{}\" '.format(' '.join(machines))
print(cmd)
os.system(cmd)
