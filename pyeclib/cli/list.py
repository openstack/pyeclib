# Copyright (c) 2025, NVIDIA
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.  THIS SOFTWARE IS
# PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
# NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import argparse
import sys

from pyeclib import cli
from pyeclib import ec_iface


def add_list_args(parser):
    parser.add_argument("-a", "--available", action="store_true",
                        help="display only available backends")
    parser.add_argument("ec_type", nargs="*", type=str,
                        help="display these backends (default: all)")


def list_command(args):
    args.ec_type = cli.expand_ec_types(args.ec_type)

    width = max(len(backend) for backend in args.ec_type)
    found = 0
    for backend in args.ec_type:
        if args.available:
            if backend in ec_iface.VALID_EC_TYPES:
                print(backend)
                found += 1
        else:
            if backend not in ec_iface.ALL_EC_TYPES:
                print(f"{backend:<{width}} unknown")
            elif backend in ec_iface.VALID_EC_TYPES:
                print(f"{backend:<{width}} available")
                found += 1
            else:
                print(f"{backend:<{width}} missing")
    return 0 if found else 1


list_description = "list availability of EC backends"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=list_description)
    add_list_args(parser)
    args = parser.parse_args()
    sys.exit(list_command(args))
