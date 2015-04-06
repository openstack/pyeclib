# Copyright (c) 2013, Kevin Greenan (kmgreen2@gmail.com)
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


# PyEClib Companion tool
# Goal: When defining an EC pool, help cluster admin make an informed choice
# between available EC implementations. Generate sample swift.conf + swift-
# ring-builder hints.
#
# Suggested features:
#
# - List the "EC types" supported - EC algorithms
# - List implementations of each EC type available on the platform
#   (dumb-software-only, software with SIMD acceleration,
#    specialized hardware, etc).
# - Benchmark each algorithm with possible implementation and display
#   performance numbers.
# - Generate sample EC policy entry (for inclusion in swift.conf) for the
#   best performing algorithm + implementation. (And optionally provide swift-
#   ring-builder hints).
#
# Suggested EC policy entry format:
#
# ======== swift.conf ============
# [storage-policy:10]
# type = erasure_coding
# name = ec_jerasure_rs_cauchy_12_2
# ec_type = jerasure_rs_cauchy
# ec_k = 12
# ec_m = 2
# ============================
#
# (ec_type values are one of those available within PyEClib)

#
# User input: Num data, num parity, average file size
# Output: Ordered list of options and their corresponding conf entries
#         (limit 10)
#

from pyeclib.ec_iface import ECDriver
import random
import string
import sys
import argparse
import time
import math


class Timer:

    def __init__(self):
        self.start_time = 0
        self.end_time = 0

    def reset(self):
        self.start_time = 0
        self.end_time = 0

    def start(self):
        self.start_time = time.time()

    def stop(self):
        self.end_time = time.time()

    def curr_delta(self):
        return self.end_time - self.start_time

    def stop_and_return(self):
        self.end_time = time.time()
        return self.curr_delta()


def nCr(n, r):
    f = math.factorial
    return f(n) / f(r) / f(n - r)


class ECScheme:

    def __init__(self, k, m, ec_type):
        self.k = k
        self.m = m
        self.ec_type = ec_type

    def __str__(self):
        return "k=%d m=%d ec_type=%s" % (self.k, self.m, self.ec_type)

valid_flat_xor_hd_3 = [(6, 6), (7, 6), (8, 6), (9, 6),
                    (10, 6), (11, 6), (12, 6), (13, 6),
                    (14, 6), (15, 6)]

valid_flat_xor_hd_4 = [(6, 6), (7, 6), (8, 6), (9, 6),
                    (10, 6), (11, 6), (12, 6), (13, 6),
                    (14, 6), (15, 6), (16, 6), (17, 6),
                    (18, 6), (19, 6), (20, 6)]


def get_viable_schemes(
        max_num_frags, minimum_rate, avg_stripe_size, fault_tolerance):

    list_of_schemes = []

    #
    # Get min_k from (minimum_rate * max_num_frags)
    #
    min_k = int(math.ceil(minimum_rate * max_num_frags))

    #
    # Get min_m from the fault tolerance
    #
    min_m = fault_tolerance

    #
    # Is not information theoretically possible
    #
    if (min_k + min_m) > max_num_frags:
        return list_of_schemes

    #
    # Iterate over EC(k, max_num_frags-k) k \in [min_k, n-min_m]
    #
    for k in range(min_k, max_num_frags - min_m + 1):
        list_of_schemes.append(
            ECScheme(k, max_num_frags - k, "jerasure_rs_vand"))

        list_of_schemes.append(
            ECScheme(k, max_num_frags - k, "jerasure_rs_cauchy"))

        #
        # The XOR codes are a little tricker
        # (only check if fault_tolerance = 2 or 3)
        #
        # Constraint for 2: k <= (m choose 2)
        # Constraint for 3: k <= (m choose 3)
        #
        # The '3' flat_xor_hd_3  (and '4' in flat_xor_hd_4) refers to the Hamming
        # distance, which means the code guarantees the reconstruction of any
        # 2 lost fragments (or 3 in the case of flat_xor_hd_4).
        #
        # So, only consider the XOR code if the fault_tolerance matches and
        # the additional constraint is met
        #
        if fault_tolerance == 2:
            max_k = nCr(max_num_frags - k, 2)
            if k <= max_k and (k, max_num_frags - k) in valid_flat_xor_hd_3:
                list_of_schemes.append(
                    ECScheme(k, max_num_frags - k, "flat_xor_hd_3"))

        if fault_tolerance == 3:
            max_k = nCr(max_num_frags - k, 3)
            if k <= max_k and (k, max_num_frags - k) in valid_flat_xor_hd_4:
                list_of_schemes.append(
                    ECScheme(k, max_num_frags - k, "flat_xor_hd_4"))

    return list_of_schemes


parser = argparse.ArgumentParser(
    description='PyECLib tool to evaluate viable EC options, benchmark them '
                'and report results with the appropriate conf entries.')
parser.add_argument(
    '-n',
    type=int,
    help='max number of fragments',
    required=True)
parser.add_argument('-f', type=int, help='fault tolerance', required=True)
parser.add_argument(
    '-r',
    type=float,
    help='minimum coding rate (num_data / num_data+num_parity)',
    required=True)
parser.add_argument('-s', type=int, help='average stripe size', required=True)
parser.add_argument(
    '-l',
    type=int,
    help='set limit on number of entries returned (default = 10)',
    default=10,
)

args = parser.parse_args(sys.argv[1:])

MB = 1024 * 1024

# Generate a buffer of size 's'
if args.s > 10 * MB:
    print("s must be smaller than 10 MB.")
    sys.exit(1)

# Instantiate the timer
timer = Timer()

return_limit = args.l

schemes = get_viable_schemes(args.n, args.r, args.s, args.f)

# Results will be List[(ec_type, throughput)]
results = []

# Num iterations
num_iterations = 10

for scheme in schemes:
    print(scheme)

    # Generate a new string for each test
    file_str = ''.join(
        random.choice(
            string.ascii_uppercase + string.digits) for x in range(args.s))

    try:
        ec_driver = ECDriver(k=scheme.k, m=scheme.m, ec_type=scheme.ec_type)
    except Exception as e:
        print("Scheme %s is not defined (%s)." % (scheme, e))
        continue

    timer.start()

    for i in range(num_iterations):
        ec_driver.encode(file_str)

    duration = timer.stop_and_return()

    results.append((scheme, duration))

    timer.reset()

print(results)
results.sort(lambda x, y: (int)((1000 * x[1]) - (1000 * y[1])))

for i in range(len(results)):
    if i > return_limit:
        break

    print("\n\nPerf Rank #%d:" % i)
    print("  ======== To Use this Policy, Copy and Paste Text (not including "
          "this header and footer) to Swift Conf ========")
    print("  type = erasure_coding")
    print("  name = %s_%d_%d" % (results[i][0].ec_type,
                                 results[i][0].k, results[i][0].m))
    print("  ec_type = %s" % results[i][0].ec_type)
    print("  ec_k = %s" % results[i][0].k)
    print("  ec_m = %s" % results[i][0].m)
    print("  ================================================================"
          "==============================================")
    results[i]
