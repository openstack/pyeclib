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
import itertools
import os
import sys

from pyeclib import cli
from pyeclib import ec_iface


def add_verify_args(parser):
    parser.add_argument("-q", "--quiet", action="store_true")
    cli.add_instance_args(parser)


def verify_command(args):
    args.ec_type = cli.expand_ec_types(args.ec_type)
    total_failures = 0
    total_corrupt = 0
    data = os.urandom(args.segment_size)
    width = max(len(ec_type) for ec_type in args.ec_type)
    print(f"Using {args.n_data} data + {args.n_parity} parity with "
          f"{args.unavailable} unavailable frags")

    for ec_type in args.ec_type:
        if ec_type not in ec_iface.ALL_EC_TYPES:
            print(f"{ec_type:<{width}} unknown")
            continue
        if ec_type not in ec_iface.VALID_EC_TYPES:
            print(f"{ec_type:<{width}} not available")
            continue
        try:
            instance = ec_iface.ECDriver(
                ec_type=ec_type,
                k=args.n_data,
                m=args.n_parity,
            )
        except ec_iface.ECDriverError:
            print(f"{ec_type:<{width}} could not be instantiated")
            continue
        frags = instance.encode(data)
        combinations, failures, corrupt = check_instance(
            instance, frags, args.unavailable, data)
        total_failures += failures
        total_corrupt += corrupt
        if corrupt:
            print(f"\x1b[91;40m{ec_type:<{width}} {combinations=}, "
                  f"{failures=}, {corrupt=}\x1b[0m")
        elif failures:
            print(f"\x1b[1;91m{ec_type:<{width}} {combinations=}, "
                  f"{failures=}, {corrupt=}\x1b[0m")
        else:
            print(f"{ec_type:<{width}} {combinations=}")

    if total_corrupt:
        return 3
    if total_failures:
        return 1
    return 0


def check_instance(instance, frags, unavailable, data):
    combinations = corrupt = failures = 0
    for to_decode in itertools.combinations(frags, len(frags) - unavailable):
        combinations += 1
        try:
            rebuilt = instance.decode(to_decode)
            if rebuilt != data:
                corrupt += 1
        except ec_iface.ECDriverError:
            failures += 1
            # Might want to log?
    return combinations, failures, corrupt


verify_description = "validate reconstructability of EC schemas"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=verify_description)
    add_verify_args(parser)
    args = parser.parse_args()
    sys.exit(verify_command(args))
