import ec_iface
from ec_iface import ec
import ec_iface.ec_pyeclib as ec_pyeclib
from ec_iface.ec import ECDriver
import pyeclib
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

ec_driver = ECDriver("ec_iface.ec_pyeclib.ECPyECLibDriver", k=args.k, m=args.m, type=args.type)

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
  
