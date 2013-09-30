import ec_iface
import ec_iface.ec
import ec_iface.ec_pyeclib as ec_pyeclib
from ec_iface.ec import ECDriver
import pyeclib
import random
import string
import sys
import os
import argparse

parser = argparse.ArgumentParser(description='Decoder for PyECLib.')
parser.add_argument('k', type=int, help='number of data elements')
parser.add_argument('m', type=int, help='number of parity elements')
parser.add_argument('type', help='EC algorithm used')
parser.add_argument('fragments', metavar='fragment', nargs='+',
                   help='fragments to decode')
parser.add_argument('filename', help='output file')

args = parser.parse_args()

print args.k
print args.m
print args.type
print args.fragments
print args.filename

ec_driver = ECDriver("ec_iface.ec_pyeclib.ECPyECLibDriver", k=args.k, m=args.m, type=args.type)

fragment_list = []

for fragment in args.fragments:
  fp = open("%s" % fragment, "r")

  fragment_list.append(fp.read())

  fp.close()
  

fp = open("%s.decoded" % args.filename, "w")

decoded_file=ec_driver.decode(fragment_list)
 
fp.write(decoded_file)

fp.close()

