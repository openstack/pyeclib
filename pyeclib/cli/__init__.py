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

from pyeclib import ec_iface


_type_abbreviations = {
    # name -> prefix
    "all": "",
    "isa-l": "isa_l_",
    "isa_l": "isa_l_",
    "isal": "isa_l_",
    "jerasure": "jerasure_",
    "flat-xor": "flat_xor_",
    "flat_xor": "flat_xor_",
    "flatxor": "flat_xor_",
    "xor": "flat_xor_",
}


def expand_ec_types(user_types):
    result = set(user_types or ["all"])
    for abbrev, prefix in _type_abbreviations.items():
        if abbrev in result:
            result.remove(abbrev)
            result.update(
                ec_type
                for ec_type in ec_iface.ALL_EC_TYPES
                if ec_type.startswith(prefix)
            )
    return sorted(result)


def add_instance_args(parser, default_segment_size=1024):
    """
    Add arguments to ``parser`` for instance instantiation.
    """
    parser.add_argument(
        "--ec-type",
        action="append",
        type=str,
    )
    parser.add_argument(
        "--n-data",
        "--ndata",
        "-k",
        metavar="K",
        type=int,
        default=10,
    )
    parser.add_argument(
        "--n-parity",
        "--nparity",
        "-m",
        metavar="M",
        type=int,
        default=5,
    )
    parser.add_argument(
        "--unavailable",
        "-u",
        metavar="N",
        type=int,
        default=2,
    )
    parser.add_argument(
        "--segment-size",
        "-s",
        metavar="BYTES",
        type=int,
        default=default_segment_size,
    )
