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

from pyeclib.cli import bench
from pyeclib.cli import check
from pyeclib.cli import list as list_cli
from pyeclib.cli import verify
from pyeclib.cli import version


def main(args=None):
    parser = argparse.ArgumentParser(
        prog="pyeclib-backend",
        description="tool to get various erasure coding information",
    )
    parser.add_argument(
        "-V",
        "--version",
        action="store_const",
        dest="func",
        const=version.version_command,
        help=version.version_description,
    )
    subparsers = parser.add_subparsers()

    version_parser = subparsers.add_parser(
        "version", help=version.version_description)
    version_parser.set_defaults(func=version.version_command)

    list_parser = subparsers.add_parser(
        "list", help=list_cli.list_description)
    list_parser.set_defaults(func=list_cli.list_command)
    list_cli.add_list_args(list_parser)

    check_parser = subparsers.add_parser(
        "check", help=check.check_description)
    check_parser.set_defaults(func=check.check_command)
    check.add_check_args(check_parser)

    verify_parser = subparsers.add_parser(
        "verify", help=verify.verify_description)
    verify_parser.set_defaults(func=verify.verify_command)
    verify.add_verify_args(verify_parser)

    bench_parser = subparsers.add_parser(
        "bench", help=bench.bench_description)
    bench_parser.set_defaults(func=bench.bench_command)
    bench.add_bench_args(bench_parser)

    parsed_args = parser.parse_args(args)
    if parsed_args.func is None:
        parser.error(
            f"the following arguments are required: "
            f"{{{','.join(subparsers.choices)}}}"
        )
    sys.exit(parsed_args.func(parsed_args))


if __name__ == "__main__":
    main()
