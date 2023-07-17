#!/usr/bin/python
#

import os,sys,datetime,re
import shlex
import subprocess
import itertools
from sys import argv
from hostnames import *

machines=hostip2

cmd = 'bash ./upload_binaries.sh \"{}\"'.format(' '.join(machines))
print(cmd)
os.system(cmd)