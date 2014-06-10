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
import tempfile
import time
import unittest

import pyeclib_c


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


class TestPyECLib(unittest.TestCase):

    def __init__(self, *args):
        self.num_datas = [12, 12, 12]
        self.num_parities = [2, 3, 4]
        self.iterations = 100

        # EC algorithm and config parameters
        self.rs_types = [("rs_vand", 16), ("rs_cauchy_orig", 4)]
        self.xor_types = [("flat_xor_4", 12, 6, 4),
                          ("flat_xor_4", 10, 5, 4),
                          ("flat_xor_3", 10, 5, 3)]

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
            tmp_file = tempfile.NamedTemporaryFile()
            tmp_file.write(buf.decode('utf-8'))
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

    def time_encode(self, num_data, num_parity, w, ec_type, file_size, iterations):
        """
        :return average encode time
        """
        timer = Timer()
        tsum = 0
        handle = pyeclib_c.init(num_data, num_parity, w, ec_type)
        whole_file_bytes = self.get_tmp_file(file_size).read()

        timer.start()
        for l in range(iterations):
            fragments = pyeclib_c.encode(handle, whole_file_bytes)
        tsum = timer.stop_and_return()

        return tsum / iterations

    def time_decode(self,
                    num_data, num_parity, w, ec_type,
                    file_size, iterations, hd):
        """
        :return 2-tuple, (success, average decode time)
        """
        timer = Timer()
        tsum = 0
        handle = pyeclib_c.init(num_data, num_parity, w, ec_type)
        whole_file_bytes = self.get_tmp_file(file_size).read()
        success = True

        fragments = pyeclib_c.encode(handle, whole_file_bytes)
        orig_fragments = fragments[:]

        for i in range(iterations):
            missing_idxs = []
            num_missing = hd - 1
            for j in range(num_missing):
                idx = random.randint(0, (num_data + num_parity) - 1)
                while idx in missing_idxs:
                    idx = random.randint(0, (num_data + num_parity) - 1)
                missing_idxs.append(idx)
                fragments[idx] = b'\0' * len(fragments[0])

            timer.start()
            decoded_fragments = pyeclib_c.decode(handle,
                                                 fragments[:num_data],
                                                 fragments[num_data:],
                                                 missing_idxs,
                                                 len(fragments[0]))
            tsum += timer.stop_and_return()

            fragments = decoded_fragments

            for j in range(num_data + num_parity):
                if orig_fragments[j] != decoded_fragments[j]:
                    success = False

                    # Output the fragments for debugging
                    #with open("orig_fragments", "wb") as fd_orig:
                    #    fd_orig.write(orig_fragments[j])
                    #with open("decoded_fragments", "wb") as fd_decoded:
                    #    fd_decoded.write(decoded_fragments[j])
                    #print(("Fragment %d was not reconstructed!!!" % j))
                    #sys.exit(2)

            decoded_fragments = None

        return success, tsum / iterations

    def time_reconstruct(self,
                         num_data, num_parity, w, ec_type,
                         file_size, iterations):
        """
        :return 2-tuple, (success, average reconstruct time)
        """
        timer = Timer()
        tsum = 0
        handle = pyeclib_c.init(num_data, num_parity, w, ec_type)
        whole_file_bytes = self.get_tmp_file(file_size).read()
        success = True

        orig_fragments = pyeclib_c.encode(handle, whole_file_bytes)

        for i in range(iterations):
            fragments = orig_fragments[:]
            num_missing = 1
            missing_idxs = []
            for j in range(num_missing):
                idx = random.randint(0, (num_data + num_parity) - 1)
                while idx in missing_idxs:
                    idx = random.randint(0, (num_data + num_parity) - 1)
                missing_idxs.append(idx)
                fragments[idx] = b'\0' * len(fragments[0])

            timer.start()
            reconstructed_fragment = pyeclib_c.reconstruct(handle,
                                                           fragments[:num_data],
                                                           fragments[num_data:],
                                                           missing_idxs,
                                                           missing_idxs[0],
                                                           len(fragments[0]))
            tsum += timer.stop_and_return()

            if orig_fragments[missing_idxs[0]] != reconstructed_fragment:
                success = False
                # Output the fragments for debugging
                #with open("orig_fragments", "wb") as fd_orig:
                #    fd_orig.write(orig_fragments[missing_idxs[0]])
                #with open("decoded_fragments", "wb") as fd_decoded:
                #    fd_decoded.write(reconstructed_fragment)
                #print(("Fragment %d was not reconstructed!!!" % missing_idxs[0]))
                #sys.exit(2)

        return success, tsum / iterations

    def get_throughput(self, avg_time, size_str):
        size_desc = size_str.split("-")
        size = float(size_desc[0])

        if size_desc[1] == 'M':
            throughput = size / avg_time
        elif size_desc[1] == 'K':
            throughput = (size / 1000.0) / avg_time

        return format(throughput, '.10g')

    def test_xor_code(self):
        for (ec_type, k, m, hd) in self.xor_types:
            print(("\nRunning tests for %s k=%d, m=%d" % (ec_type, k, m)))

            type_str = "%s" % (ec_type)

            for size_str in self.sizes:
                avg_time = self.time_encode(k, m, 32,
                                            type_str, size_str,
                                            self.iterations)
                print("Encode (%s): %s" %
                      (size_str, self.get_throughput(avg_time, size_str)))

            for size_str in self.sizes:
                success, avg_time = self.time_decode(k, m, 32,
                                                     type_str, size_str,
                                                     self.iterations, 3)
                self.assertTrue(success)
                print("Decode (%s): %s" %
                      (size_str, self.get_throughput(avg_time, size_str)))

            for size_str in self.sizes:
                success, avg_time = self.time_reconstruct(k, m, 32,
                                                          type_str, size_str,
                                                          self.iterations)
                self.assertTrue(success)
                print("Reconstruct (%s): %s" %
                      (size_str, self.get_throughput(avg_time, size_str)))

    def _test_get_fragment_partition(self,
                                     num_data, num_parity, w, ec_type,
                                     file_size, iterations):
        """
        :return boolean, True if all tests passed
        """
        handle = pyeclib_c.init(num_data, num_parity, w, ec_type)
        whole_file_bytes = self.get_tmp_file(file_size).read()
        success = True

        fragments = pyeclib_c.encode(handle, whole_file_bytes)

        for i in range(iterations):
            missing_fragments = random.sample(fragments, 3)
            avail_fragments = fragments[:]
            missing_fragment_idxs = []
            for missing_frag in missing_fragments:
                missing_fragment_idxs.append(fragments.index(missing_frag))
                avail_fragments.remove(missing_frag)

            (data_frags, parity_frags, missing_idxs) =\
                pyeclib_c.get_fragment_partition(handle, avail_fragments)

            missing_fragment_idxs.sort()
            missing_idxs.sort()

            if missing_fragment_idxs != missing_idxs:
                success = False
                print(("Missing idx mismatch in test_get_fragment_partition: "
                       "%s != %s\n" % (missing_fragment_idxs, missing_idxs)))
                #sys.exit()

            decoded_frags = pyeclib_c.decode(handle, data_frags, parity_frags,
                                             missing_idxs, len(data_frags[0]))

            return success

    def _test_fragments_to_string(self,
                                  num_data, num_parity, w, ec_type,
                                  file_size):
        """
        :return boolean, True if all tests passed
        """
        handle = pyeclib_c.init(num_data, num_parity, w, ec_type)
        whole_file_bytes = self.get_tmp_file(file_size).read()
        success = True

        fragments = pyeclib_c.encode(handle, whole_file_bytes)
        concat_str = pyeclib_c.fragments_to_string(handle, fragments[:num_data])

        if concat_str != whole_file_bytes:
            success = False
            print(("String does not equal the original string "
                   "(len(orig) = %d, len(new) = %d\n" %
                   (len(whole_file_bytes), len(concat_str))))

        return success

    def _test_get_required_fragments(self, num_data, num_parity, w, ec_type):
        """
        :return boolean, True if all tests passed
        """
        handle = pyeclib_c.init(num_data, num_parity, w, ec_type)
        success = True

        #
        # MDS codes need any k fragments
        #
        if ec_type in ["rs_vand", "rs_cauchy_orig"]:
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
                missing_fragments)

            if expected_fragments != required_fragments:
                success = False
                print(("Unexpected required fragments list "
                       "(exp != req): %s != %s" %
                       (expected_fragments, required_fragments)))
                #sys.exit(2)

        return success

    def test_rs_code(self):
        for (ec_type, w) in self.rs_types:
            print(("\nRunning tests for %s w=%d" % (ec_type, w)))

            for i in range(len(self.num_datas)):
                for size_str in self.sizes:
                    success = self._test_get_fragment_partition(self.num_datas[i],
                                                                self.num_parities[i],
                                                                w, ec_type, size_str,
                                                                self.iterations)
                    self.assertTrue(success)

            for i in range(len(self.num_datas)):
                for size_str in self.sizes:
                    success = self._test_fragments_to_string(self.num_datas[i],
                                                             self.num_parities[i],
                                                             w, ec_type, size_str)
                    self.assertTrue(success)

            for i in range(len(self.num_datas)):
                success = self._test_get_required_fragments(self.num_datas[i],
                                                            self.num_parities[i],
                                                            w, ec_type)
                self.assertTrue(success)

            for i in range(len(self.num_datas)):
                for size_str in self.sizes:
                    avg_time = self.time_encode(self.num_datas[i],
                                                self.num_parities[i],
                                                w, ec_type, size_str,
                                                self.iterations)
                    print(("Encode (%s): %s" %
                           (size_str, self.get_throughput(avg_time, size_str))))

            for i in range(len(self.num_datas)):
                for size_str in self.sizes:
                    success, avg_time = self.time_decode(self.num_datas[i],
                                                         self.num_parities[i],
                                                         w, ec_type, size_str,
                                                         self.iterations,
                                                         self.num_parities[i] + 1)
                    self.assertTrue(success)
                    print(("Decode (%s): %s" %
                           (size_str, self.get_throughput(avg_time, size_str))))

            for i in range(len(self.num_datas)):
                for size_str in self.sizes:
                    success, avg_time = self.time_reconstruct(self.num_datas[i],
                                                              self.num_parities[i],
                                                              w, ec_type, size_str,
                                                              self.iterations)
                    self.assertTrue(success)
                    print(("Reconstruct (%s): %s" %
                           (size_str, self.get_throughput(avg_time, size_str))))


if __name__ == "__main__":
    unittest.main()

