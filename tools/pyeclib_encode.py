
# Copyright (c) 2013, Kevin Greenan (kmgreen2@gmail.com)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.  THIS SOFTWARE IS PROVIDED BY
# THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import pyeclib
from pyeclib.ec_iface import ECDriver
import random
import string
import sys
import os
import argparse

parser = argparse.ArgumentParser(description='Encoder for PyECLib.')
parser.add_argument('k', type=int, help='number of data elements')
parser.add_argument('m', type=int, help='number of parity elements')
parser.add_argument('type', help='EC algorithm used')
parser.add_argument('file_dir', help='directory with the file')
parser.add_argument('filename', help='file to encode')
parser.add_argument('fragment_dir', help='directory to drop encoded fragments')

args = parser.parse_args()

print args.k
print args.m
print args.type
print args.filename

ec_driver = ECDriver("pyeclib.core.ECPyECLibDriver", k=args.k, m=args.m, type=args.type)

fp = open("%s/%s" % (args.file_dir, args.filename), "r")
 
whole_file_str = fp.read()
   
fragments=ec_driver.encode(whole_file_str)

fp.close()

i = 0
for fragment in fragments:
  fp = open("%s/%s.%d" % (args.fragment_dir, args.filename, i), "w")
  fp.write(fragment)
  fp.close()
  i += 1
  
