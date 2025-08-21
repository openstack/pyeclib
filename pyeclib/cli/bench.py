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
import os
import random
import time

from pyeclib import cli
from pyeclib import ec_iface


def add_bench_args(parser):
    parser.add_argument("-e", "--encode", action="store_true")
    parser.add_argument("-d", "--decode", action="store_true")
    cli.add_instance_args(parser, default_segment_size=2**20)
    parser.add_argument("--iterations", "-i", type=int, default=200)


def bench_command(args):
    args.ec_type = cli.expand_ec_types(args.ec_type)
    data = os.urandom(args.segment_size + args.iterations)
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
        frags = instance.encode(data[:args.segment_size])

        if args.encode or not args.decode:
            start = time.time()
            for i in range(args.iterations):
                _ = instance.encode(data[i:i + args.segment_size])
            dt = time.time() - start
            mb_encoded = args.iterations * args.segment_size / (2 ** 20)
            print(f'{ec_type} (encode): {mb_encoded / dt:.1f}MB/s')

        if args.decode or not args.encode:
            start = time.time()
            for i in range(args.iterations):
                data_frags = random.sample(
                    frags[:args.n_data],
                    args.n_data - args.unavailable,
                )
                if ec_type.startswith('flat_xor'):
                    # The math is actually more complicated than this, but ...
                    parity_frags = frags[args.n_data:]
                else:
                    parity_frags = random.sample(
                        frags[args.n_data:],
                        args.unavailable,
                    )
                _ = instance.decode(data_frags + parity_frags)
            dt = time.time() - start
            mb_encoded = args.iterations * args.segment_size / (2 ** 20)
            print(f'{ec_type} (decode): {mb_encoded / dt:.1f}MB/s')


bench_description = "benchmark EC schemas"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=bench_description)
    add_bench_args(parser)
    args = parser.parse_args()
    bench_command(args)
