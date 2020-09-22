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

import random
from string import ascii_letters
import sys
import tempfile
import time
import unittest

import pyeclib_c
from pyeclib.ec_iface import PyECLib_EC_Types
from pyeclib.ec_iface import VALID_EC_TYPES


class Timer:

    def __init__(self):
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


def require_backend(backend):
    return unittest.skipIf(backend not in VALID_EC_TYPES,
                           "%s backend is not available" % backend)


class TestPyECLib(unittest.TestCase):

    def __init__(self, *args):
        self.num_datas = [12, 12, 12]
        self.num_parities = [2, 3, 4]
        self.iterations = 100

        # EC algorithm and config parameters
        self.rs_types = [(PyECLib_EC_Types.jerasure_rs_vand),
                         (PyECLib_EC_Types.jerasure_rs_cauchy),
                         (PyECLib_EC_Types.isa_l_rs_vand),
                         (PyECLib_EC_Types.liberasurecode_rs_vand),
                         (PyECLib_EC_Types.isa_l_rs_cauchy)]
        self.xor_types = [(PyECLib_EC_Types.flat_xor_hd, 12, 6, 4),
                          (PyECLib_EC_Types.flat_xor_hd, 10, 5, 4),
                          (PyECLib_EC_Types.flat_xor_hd, 10, 5, 3)]
        self.shss = [(PyECLib_EC_Types.shss, 6, 3),
                     (PyECLib_EC_Types.shss, 10, 4),
                     (PyECLib_EC_Types.shss, 20, 4),
                     (PyECLib_EC_Types.shss, 11, 7)]
        self.libphazr = [(PyECLib_EC_Types.libphazr, 4, 4)]

        # Input temp files for testing
        self.sizes = ["101-K", "202-K", "303-K"]
        self.files = {}
        self._create_tmp_files()

        unittest.TestCase.__init__(self, *args)

    def _create_tmp_files(self):
        """
        Create the temporary files needed for testing.  Use the tempfile
        package so that the files will be automatically removed during
        garbage collection.
        """
        for size_str in self.sizes:
            # Determine the size of the file to create
            size_desc = size_str.split("-")
            size = int(size_desc[0])
            if size_desc[1] == 'M':
                size *= 1000000
            elif size_desc[1] == 'K':
                size *= 1000

            # Create the dictionary of files to test with
            buf = ''.join(random.choice(ascii_letters) for i in range(size))
            if sys.version_info >= (3,):
                buf = buf.encode('ascii')
            tmp_file = tempfile.NamedTemporaryFile('w+b')
            tmp_file.write(buf)
            self.files[size_str] = tmp_file

    def get_tmp_file(self, name):
        """
        Acquire a temp file from the dictionary of pre-built, random files
        with the seek position to the head of the file.
        """
        tmp_file = self.files.get(name, None)

        if tmp_file:
            tmp_file.seek(0, 0)

        return tmp_file

    def setUp(self):
        # Ensure that the file offset is set to the head of the file
        for _, tmp_file in self.files.items():
            tmp_file.seek(0, 0)

    def tearDown(self):
        pass

    def iter_available_types(self, ec_types):
        found_one = False
        for ec_type in ec_types:
            if ec_type.name not in VALID_EC_TYPES:
                continue
            found_one = True
            yield ec_type
        if not found_one:
            type_list = ', '.join(t.name for t in ec_types)
            raise unittest.SkipTest('No backend available in types: %r' %
                                    type_list)

    def time_encode(self, num_data, num_parity, ec_type, hd,
                    file_size, iterations):
        """
        :return average encode time
        """
        timer = Timer()
        handle = pyeclib_c.init(num_data, num_parity, ec_type, hd)
        whole_file_bytes = self.get_tmp_file(file_size).read()

        timer.start()
        for i in range(iterations):
            pyeclib_c.encode(handle, whole_file_bytes)
        tsum = timer.stop_and_return()

        return tsum / iterations

    def time_decode(self,
                    num_data, num_parity, ec_type, hd,
                    file_size, iterations):
        """
        :return 2-tuple, (success, average decode time)
        """
        timer = Timer()
        tsum = 0
        handle = pyeclib_c.init(num_data, num_parity, ec_type, hd)
        whole_file_bytes = self.get_tmp_file(file_size).read()
        success = True

        fragments = pyeclib_c.encode(handle, whole_file_bytes)
        orig_fragments = fragments[:]

        for i in range(iterations):
            num_missing = hd - 1
            for j in range(num_missing):
                num_frags_left = len(fragments)
                idx = random.randint(0, num_frags_left - 1)
                fragments.pop(idx)

            timer.start()
            decoded_file_bytes = pyeclib_c.decode(handle,
                                                  fragments,
                                                  len(fragments[0]))
            tsum += timer.stop_and_return()

            fragments = orig_fragments[:]

            if whole_file_bytes != decoded_file_bytes:
                success = False

        return success, tsum / iterations

    def time_range_decode(self,
                          num_data, num_parity, ec_type, hd,
                          file_size, iterations):
        """
        :return 2-tuple, (success, average decode time)
        """
        timer = Timer()
        tsum = 0
        handle = pyeclib_c.init(num_data, num_parity, ec_type, hd)
        whole_file_bytes = self.get_tmp_file(file_size).read()
        success = True

        begins = [int(random.randint(0, len(whole_file_bytes) - 1))
                  for i in range(3)]
        ends = [int(random.randint(begins[i], len(whole_file_bytes)))
                for i in range(3)]

        ranges = list(zip(begins, ends))

        fragments = pyeclib_c.encode(handle, whole_file_bytes)
        orig_fragments = fragments[:]

        for i in range(iterations):
            num_missing = hd - 1
            for j in range(num_missing):
                num_frags_left = len(fragments)
                idx = random.randint(0, num_frags_left - 1)
                fragments.pop(idx)

            timer.start()
            decoded_file_ranges = pyeclib_c.decode(handle,
                                                   fragments,
                                                   len(fragments[0]),
                                                   ranges)
            tsum += timer.stop_and_return()

            fragments = orig_fragments[:]

            range_offset = 0
            for r in ranges:
                if whole_file_bytes[
                        r[0]: r[1] + 1] != decoded_file_ranges[range_offset]:
                    success = False
                range_offset += 1

        return success, tsum / iterations

    def time_reconstruct(self,
                         num_data, num_parity, ec_type, hd,
                         file_size, iterations):
        """
        :return 2-tuple, (success, average reconstruct time)
        """
        timer = Timer()
        tsum = 0
        handle = pyeclib_c.init(num_data, num_parity, ec_type, hd)
        whole_file_bytes = self.get_tmp_file(file_size).read()
        success = True

        orig_fragments = pyeclib_c.encode(handle, whole_file_bytes)

        for i in range(iterations):
            fragments = orig_fragments[:]
            num_missing = 1
            missing_idxs = []
            for j in range(num_missing):
                num_frags_left = len(fragments)
                idx = random.randint(0, num_frags_left - 1)
                while idx in missing_idxs:
                    idx = random.randint(0, num_frags_left - 1)
                missing_idxs.append(idx)
                fragments.pop(idx)

            timer.start()
            reconstructed_fragment = pyeclib_c.reconstruct(handle,
                                                           fragments,
                                                           len(fragments[0]),
                                                           missing_idxs[0])
            tsum += timer.stop_and_return()

            if orig_fragments[missing_idxs[0]] != reconstructed_fragment:
                success = False
                # Output the fragments for debugging
                with open("orig_fragments", "wb") as fd_orig:
                    fd_orig.write(orig_fragments[missing_idxs[0]])
                with open("decoded_fragments", "wb") as fd_decoded:
                    fd_decoded.write(reconstructed_fragment)
                print("Fragment %d was not reconstructed!!!" % missing_idxs[0])
                sys.exit(2)

        return success, tsum / iterations

    def get_throughput(self, avg_time, size_str):
        size_desc = size_str.split("-")
        size = float(size_desc[0])

        if avg_time == 0:
            return '? (test finished too fast to calculate throughput)'

        if size_desc[1] == 'M':
            throughput = size / avg_time
        elif size_desc[1] == 'K':
            throughput = (size / 1000.0) / avg_time

        return format(throughput, '.10g')

    @require_backend("flat_xor_hd_3")
    def test_xor_code(self):
        for (ec_type, k, m, hd) in self.xor_types:
            print("\nRunning tests for flat_xor_hd k=%d, m=%d, hd=%d" %
                  (k, m, hd))

            for size_str in self.sizes:
                avg_time = self.time_encode(k, m, ec_type.value, hd,
                                            size_str,
                                            self.iterations)
                print("Encode (%s): %s" %
                      (size_str, self.get_throughput(avg_time, size_str)))

            for size_str in self.sizes:
                success, avg_time = self.time_decode(k, m, ec_type.value, hd,
                                                     size_str,
                                                     self.iterations)
                self.assertTrue(success)
                print("Decode (%s): %s" %
                      (size_str, self.get_throughput(avg_time, size_str)))

            for size_str in self.sizes:
                success, avg_time = self.time_reconstruct(
                    k, m, ec_type.value, hd, size_str, self.iterations)
                self.assertTrue(success)
                print("Reconstruct (%s): %s" %
                      (size_str, self.get_throughput(avg_time, size_str)))

    @require_backend("shss")
    def test_shss(self):
        for (ec_type, k, m) in self.shss:
            print(("\nRunning tests for %s k=%d, m=%d" % (ec_type, k, m)))

            success = self._test_get_required_fragments(k, m, ec_type)
            self.assertTrue(success)

            for size_str in self.sizes:
                avg_time = self.time_encode(k, m, ec_type.value, 0,
                                            size_str,
                                            self.iterations)
                print("Encode (%s): %s" %
                      (size_str, self.get_throughput(avg_time, size_str)))

            for size_str in self.sizes:
                success, avg_time = self.time_decode(k, m, ec_type.value, 0,
                                                     size_str,
                                                     self.iterations)
                self.assertTrue(success)
                print("Decode (%s): %s" %
                      (size_str, self.get_throughput(avg_time, size_str)))

            for size_str in self.sizes:
                success, avg_time = self.time_reconstruct(
                    k, m, ec_type.value, 0, size_str, self.iterations)
                self.assertTrue(success)
                print("Reconstruct (%s): %s" %
                      (size_str, self.get_throughput(avg_time, size_str)))

    @require_backend("libphazr")
    def test_libphazr(self):
        for (ec_type, k, m) in self.libphazr:
            print(("\nRunning tests for %s k=%d, m=%d" % (ec_type, k, m)))

            success = self._test_get_required_fragments(k, m, ec_type)
            self.assertTrue(success)

            for size_str in self.sizes:
                avg_time = self.time_encode(k, m, ec_type.value, 0,
                                            size_str,
                                            self.iterations)
                print("Encode (%s): %s" %
                      (size_str, self.get_throughput(avg_time, size_str)))

            for size_str in self.sizes:
                success, avg_time = self.time_decode(k, m, ec_type.value, 0,
                                                     size_str,
                                                     self.iterations)
                self.assertTrue(success)
                print("Decode (%s): %s" %
                      (size_str, self.get_throughput(avg_time, size_str)))

            for size_str in self.sizes:
                success, avg_time = self.time_reconstruct(
                    k, m, ec_type.value, 0, size_str, self.iterations)
                self.assertTrue(success)
                print("Reconstruct (%s): %s" %
                      (size_str, self.get_throughput(avg_time, size_str)))

    def _test_get_required_fragments(self, num_data, num_parity, ec_type):
        """
        :return boolean, True if all tests passed
        """
        handle = pyeclib_c.init(num_data, num_parity, ec_type.value)
        success = True

        #
        # MDS codes need any k fragments
        #
        if ec_type in ["jerasure_rs_vand", "jerasure_rs_cauchy",
                       "liberasurecode_rs_vand"]:
            expected_fragments = [i for i in range(num_data + num_parity)]
            missing_fragments = []

            #
            # Remove between 1 and num_parity
            #
            for i in range(random.randint(0, num_parity - 1)):
                missing_fragment = random.sample(expected_fragments, 1)[0]
                missing_fragments.append(missing_fragment)
                expected_fragments.remove(missing_fragment)

            expected_fragments = expected_fragments[:num_data]
            required_fragments = pyeclib_c.get_required_fragments(
                handle,
                missing_fragments, [])

            if expected_fragments != required_fragments:
                success = False
                print("Unexpected required fragments list "
                      "(exp != req): %s != %s" % (
                          expected_fragments, required_fragments))

        return success

    def test_codes(self):
        for ec_type in self.iter_available_types(self.rs_types):
            for i in range(len(self.num_datas)):
                success = self._test_get_required_fragments(
                    self.num_datas[i], self.num_parities[i], ec_type)
                self.assertTrue(success)

            for i in range(len(self.num_datas)):
                for size_str in self.sizes:
                    avg_time = self.time_encode(
                        self.num_datas[i], self.num_parities[i], ec_type.value,
                        self.num_parities[i] + 1, size_str, self.iterations)

                    print("Encode (%s): %s" %
                          (size_str, self.get_throughput(avg_time, size_str)))

            for i in range(len(self.num_datas)):
                for size_str in self.sizes:
                    success, avg_time = self.time_decode(
                        self.num_datas[i], self.num_parities[i], ec_type.value,
                        self.num_parities[i] + 1, size_str, self.iterations)

                    self.assertTrue(success)
                    print("Decode (%s): %s" %
                          (size_str, self.get_throughput(avg_time, size_str)))

            for i in range(len(self.num_datas)):
                for size_str in self.sizes:
                    success, avg_time = self.time_range_decode(
                        self.num_datas[i], self.num_parities[i], ec_type.value,
                        self.num_parities[i] + 1, size_str, self.iterations)

                    self.assertTrue(success)
                    print("Range Decode (%s): %s" %
                          (size_str, self.get_throughput(avg_time, size_str)))

            for i in range(len(self.num_datas)):
                for size_str in self.sizes:
                    success, avg_time = self.time_reconstruct(
                        self.num_datas[i], self.num_parities[i], ec_type.value,
                        self.num_parities[i] + 1, size_str, self.iterations)
                    self.assertTrue(success)
                    print("Reconstruct (%s): %s" %
                          (size_str, self.get_throughput(avg_time, size_str)))


if __name__ == "__main__":
    unittest.main()
