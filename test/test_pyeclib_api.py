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

import os
import random
from string import ascii_letters, ascii_uppercase, digits
import sys
import tempfile
import unittest

from distutils.version import StrictVersion
from itertools import combinations
import six

import pyeclib.ec_iface
from pyeclib.ec_iface import ECBackendNotSupported
from pyeclib.ec_iface import ECDriver
from pyeclib.ec_iface import ECDriverError
from pyeclib.ec_iface import ECInsufficientFragments
from pyeclib.ec_iface import ECInvalidFragmentMetadata
from pyeclib.ec_iface import ECInvalidParameter
from pyeclib.ec_iface import PyECLib_EC_Types
from pyeclib.ec_iface import ALL_EC_TYPES
from pyeclib.ec_iface import VALID_EC_TYPES
from pyeclib.ec_iface import LIBERASURECODE_VERSION
import resource


if sys.version < '3':
    def b2i(b):
        return ord(b)
else:
    def b2i(b):
        return b


class TestNullDriver(unittest.TestCase):

    def setUp(self):
        self.null_driver = ECDriver(
            library_import_str="pyeclib.core.ECNullDriver", k=8, m=2)

    def tearDown(self):
        pass

    def test_null_driver(self):
        self.null_driver.encode('')
        self.null_driver.decode([])


class TestStripeDriver(unittest.TestCase):

    def setUp(self):
        self.stripe_driver = ECDriver(
            library_import_str="pyeclib.core.ECStripingDriver", k=8, m=0)

    def tearDown(self):
        pass


class TestPyECLibDriver(unittest.TestCase):

    def __init__(self, *args):
        # Create the temp files needed for testing
        self.file_sizes = ["100-K"]
        self.files = {}
        self.num_iterations = 100
        self._create_tmp_files()

        unittest.TestCase.__init__(self, *args)

    def _create_tmp_files(self):
        """
        Create the temporary files needed for testing.  Use the tempfile
        package so that the files will be automatically removed during
        garbage collection.
        """
        for size_str in self.file_sizes:
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
            tmp_file = tempfile.NamedTemporaryFile()
            tmp_file.write(buf)
            self.files[size_str] = tmp_file

    def setUp(self):
        # Ensure that the file offset is set to the head of the file
        for _, tmp_file in self.files.items():
            tmp_file.seek(0, 0)

    def tearDown(self):
        pass

    def test_missing_required_args(self):
        # missing ec_type
        with self.assertRaises(ECDriverError) as err_context:
            ECDriver(k=1, m=1)
        self.assertEqual(str(err_context.exception),
                         "Invalid Argument: either ec_type or "
                         "library_import_str must be provided")

        for ec_type in VALID_EC_TYPES:
            # missing k
            with self.assertRaises(ECDriverError) as err_context:
                ECDriver(ec_type=ec_type, m=1)
            self.assertEqual(str(err_context.exception),
                             "Invalid Argument: k is required")

            # missing m
            with self.assertRaises(ECDriverError) as err_context:
                ECDriver(ec_type=ec_type, k=1)
            self.assertEqual(str(err_context.exception),
                             "Invalid Argument: m is required")

    def test_invalid_km_args(self):
        for ec_type in VALID_EC_TYPES:
            with self.assertRaises(ECDriverError) as err_context:
                # k is smaller than 1
                ECDriver(ec_type=ec_type, k=-100, m=1)
            self.assertEqual(str(err_context.exception),
                             "Invalid number of data fragments (k)")

            with self.assertRaises(ECDriverError) as err_context:
                # m is smaller than 1
                ECDriver(ec_type=ec_type, k=1, m=-100)
            self.assertEqual(str(err_context.exception),
                             "Invalid number of parity fragments (m)")

    def test_valid_ec_types(self):
        # Build list of available types and compare to VALID_EC_TYPES
        available_ec_types = []
        for _type in ALL_EC_TYPES:
            try:
                if _type == 'shss':
                    _k = 10
                    _m = 4
                elif _type == 'libphazr':
                    _k = 4
                    _m = 4
                else:
                    _k = 10
                    _m = 5
                ECDriver(k=_k, m=_m, ec_type=_type, validate=True)
                available_ec_types.append(_type)
            except Exception:
                # ignore any errors, assume backend not available
                pass
        self.assertEqual(available_ec_types, VALID_EC_TYPES)

    def test_valid_algo(self):
        print("")
        for _type in ALL_EC_TYPES:
            # Check if this algo works
            if _type not in VALID_EC_TYPES:
                print("Skipping test for %s backend" % _type)
                continue
            try:
                if _type == 'shss':
                    ECDriver(k=10, m=4, ec_type=_type)
                elif _type == 'libphazr':
                    ECDriver(k=4, m=4, ec_type=_type)
                else:
                    ECDriver(k=10, m=5, ec_type=_type)
            except ECDriverError:
                self.fail("%s algorithm not supported" % _type)

        self.assertRaises(ECBackendNotSupported, ECDriver, k=10, m=5,
                          ec_type="invalid_algo")

    def get_pyeclib_testspec(self, csum="none"):
        pyeclib_drivers = []
        _type1 = 'jerasure_rs_vand'
        if _type1 in VALID_EC_TYPES:
            pyeclib_drivers.append(ECDriver(k=12, m=2, ec_type=_type1,
                                            chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=11, m=2, ec_type=_type1,
                                            chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=10, m=2, ec_type=_type1,
                                            chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=8, m=4, ec_type=_type1,
                                            chksum_type=csum))
        _type2 = 'liberasurecode_rs_vand'
        if _type2 in VALID_EC_TYPES:
            pyeclib_drivers.append(ECDriver(k=12, m=2, ec_type=_type2,
                                            chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=11, m=2, ec_type=_type2,
                                            chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=10, m=2, ec_type=_type2,
                                            chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=8, m=4, ec_type=_type2,
                                            chksum_type=csum))
        _type3_1 = 'flat_xor_hd'
        if _type3_1 in VALID_EC_TYPES:
            pyeclib_drivers.append(ECDriver(k=12, m=6, ec_type=_type3_1,
                                            chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=10, m=5, ec_type=_type3_1,
                                            chksum_type=csum))
        _type3_2 = 'flat_xor_hd_4'
        if _type3_2 in VALID_EC_TYPES:
            pyeclib_drivers.append(ECDriver(k=12, m=6, ec_type=_type3_2,
                                            chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=10, m=5, ec_type=_type3_2,
                                            chksum_type=csum))
        _type4 = 'shss'
        if _type4 in VALID_EC_TYPES:
            pyeclib_drivers.append(ECDriver(k=10, m=4, ec_type=_type4,
                                            chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=20, m=4, ec_type=_type4,
                                            chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=11, m=7, ec_type=_type4,
                                            chksum_type=csum))

        _type5 = 'isa_l_rs_vand'
        if _type5 in VALID_EC_TYPES:
            pyeclib_drivers.append(ECDriver(k=12, m=2, ec_type=_type5,
                                   chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=11, m=2, ec_type=_type5,
                                   chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=10, m=2, ec_type=_type5,
                                   chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=8, m=4, ec_type=_type5,
                                   chksum_type=csum))
        _type6 = 'isa_l_rs_cauchy'
        if _type6 in VALID_EC_TYPES:
            pyeclib_drivers.append(ECDriver(k=12, m=2, ec_type=_type6,
                                   chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=11, m=2, ec_type=_type6,
                                   chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=10, m=2, ec_type=_type6,
                                   chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=8, m=4, ec_type=_type6,
                                   chksum_type=csum))
            pyeclib_drivers.append(ECDriver(k=11, m=7, ec_type=_type6,
                                   chksum_type=csum))

        _type7 = 'libphazr'
        if _type7 in VALID_EC_TYPES:
            pyeclib_drivers.append(ECDriver(k=4, m=4, ec_type=_type7,
                                   chksum_type=csum))
        return pyeclib_drivers

    def test_small_encode(self):
        pyeclib_drivers = self.get_pyeclib_testspec()
        encode_strs = [b"a", b"hello", b"hellohyhi", b"yo"]

        for pyeclib_driver in pyeclib_drivers:
            for encode_str in encode_strs:
                encoded_fragments = pyeclib_driver.encode(encode_str)
                decoded_str = pyeclib_driver.decode(encoded_fragments)

                self.assertTrue(decoded_str == encode_str)

    def test_encode_invalid_params(self):
        pyeclib_drivers = self.get_pyeclib_testspec()
        encode_args = [u'\U0001F0A1', 3, object(), None, {}, []]

        for pyeclib_driver in pyeclib_drivers:
            for encode_str in encode_args:
                with self.assertRaises(ECInvalidParameter):
                    pyeclib_driver.encode(encode_str)

    def test_attribute_error_in_the_error_handling(self):
        pyeclib_drivers = self.get_pyeclib_testspec()
        self.assertGreater(len(pyeclib_drivers), 0)
        pyeclib_driver = pyeclib_drivers[0]

        del pyeclib.ec_iface.ECInvalidParameter
        try:
            with self.assertRaises(AttributeError):  # !!
                pyeclib_driver.encode(3)
        finally:
            pyeclib.ec_iface.ECInvalidParameter = ECInvalidParameter

    def test_import_error_in_the_error_handling(self):
        pyeclib_drivers = self.get_pyeclib_testspec()
        self.assertGreater(len(pyeclib_drivers), 0)
        pyeclib_driver = pyeclib_drivers[0]

        from six.moves import builtins
        real_import = builtins.__import__

        def fake_import(*a, **kw):
            raise ImportError

        builtins.__import__ = fake_import
        try:
            with self.assertRaises(ImportError):  # !!
                pyeclib_driver.encode(3)
        finally:
            builtins.__import__ = real_import

    def test_decode_reconstruct_with_fragment_iterator(self):
        pyeclib_drivers = self.get_pyeclib_testspec()
        encode_strs = [b"a", b"hello", b"hellohyhi", b"yo"]

        for pyeclib_driver in pyeclib_drivers:
            for encode_str in encode_strs:
                encoded_fragments = pyeclib_driver.encode(encode_str)

                idxs_to_remove = random.sample(range(
                    pyeclib_driver.k + pyeclib_driver.m), 2)
                available_fragments = encoded_fragments[:]
                for i in sorted(idxs_to_remove, reverse=True):
                    available_fragments.pop(i)

                frag_iter = iter(available_fragments)
                decoded_str = pyeclib_driver.decode(frag_iter)
                self.assertEqual(decoded_str, encode_str)

                # Since the iterator is exhausted, we can't decode again
                with self.assertRaises(ECDriverError) as exc_mgr:
                    pyeclib_driver.decode(frag_iter)
                self.assertEqual(
                    'Invalid fragment payload in ECPyECLibDriver.decode',
                    str(exc_mgr.exception))

                frag_iter = iter(available_fragments)
                reconstructed_fragments = pyeclib_driver.reconstruct(
                    frag_iter, idxs_to_remove)
                self.assertEqual(len(reconstructed_fragments),
                                 len(idxs_to_remove))
                for i, data in zip(idxs_to_remove, reconstructed_fragments):
                    self.assertEqual(data, encoded_fragments[i])

                # Since the iterator is exhausted, we can't decode again
                with self.assertRaises(ECDriverError) as exc_mgr:
                    pyeclib_driver.reconstruct(frag_iter, idxs_to_remove)
                self.assertEqual(
                    'Invalid fragment payload in ECPyECLibDriver.reconstruct',
                    str(exc_mgr.exception))

    def check_metadata_formatted(self, k, m, ec_type, chksum_type):

        if ec_type not in VALID_EC_TYPES:
            return

        filesize = 1024 * 1024 * 3
        file_str = ''.join(random.choice(ascii_letters)
                           for i in range(filesize))
        file_bytes = file_str.encode('utf-8')

        pyeclib_driver = ECDriver(k=k, m=m, ec_type=ec_type,
                                  chksum_type=chksum_type)

        fragments = pyeclib_driver.encode(file_bytes)

        f = 0
        for fragment in fragments:
            metadata = pyeclib_driver.get_metadata(fragment, 1)
            if 'index' in metadata:
                self.assertEqual(metadata['index'], f)
            else:
                self.assertTrue(False)

            if 'chksum_mismatch' in metadata:
                self.assertEqual(metadata['chksum_mismatch'], 0)
            else:
                self.assertTrue(False)

            if 'backend_id' in metadata:
                self.assertEqual(metadata['backend_id'], ec_type)
            else:
                self.assertTrue(False)

            if 'orig_data_size' in metadata:
                self.assertEqual(metadata['orig_data_size'], 3145728)
            else:
                self.assertTrue(False)

            if 'chksum_type' in metadata:
                self.assertEqual(metadata['chksum_type'], 'crc32')
            else:
                self.assertTrue(False)

            if 'backend_version' not in metadata:
                self.assertTrue(False)

            if 'chksum' not in metadata:
                self.assertTrue(False)

            if 'size' not in metadata:
                self.assertTrue(False)

            f += 1

    def test_get_metadata_formatted(self):
        self.check_metadata_formatted(12, 2, "jerasure_rs_vand",
                                      "inline_crc32")
        self.check_metadata_formatted(12, 2, "liberasurecode_rs_vand",
                                      "inline_crc32")
        self.check_metadata_formatted(8, 4, "liberasurecode_rs_vand",
                                      "inline_crc32")

    def test_verify_fragment_inline_chksum_fail(self):
        pyeclib_drivers = self.get_pyeclib_testspec("inline_crc32")
        filesize = 1024 * 1024 * 3
        file_str = ''.join(random.choice(ascii_letters)
                           for i in range(filesize))
        file_bytes = file_str.encode('utf-8')

        for pyeclib_driver in pyeclib_drivers:
            fragments = pyeclib_driver.encode(file_bytes)

            fragment_metadata_list = []

            first_fragment_to_corrupt = random.randint(0, len(fragments))
            num_to_corrupt = 2
            fragments_to_corrupt = [
                (first_fragment_to_corrupt + i) % len(fragments)
                for i in range(num_to_corrupt + 1)]
            fragments_to_corrupt.sort()

            i = 0
            for fragment in fragments:
                if i in fragments_to_corrupt:
                    corrupted_fragment = fragment[:100] +\
                        (str(chr((b2i(fragment[100]) + 0x1)
                                 % 128))).encode('utf-8') + fragment[101:]
                    fragment_metadata_list.append(
                        pyeclib_driver.get_metadata(corrupted_fragment))
                else:
                    fragment_metadata_list.append(
                        pyeclib_driver.get_metadata(fragment))
                i += 1

            expected_ret_value = {"status": -206,
                                  "reason": "Bad checksum",
                                  "bad_fragments": fragments_to_corrupt}
            self.assertEqual(
                pyeclib_driver.verify_stripe_metadata(fragment_metadata_list),
                expected_ret_value)

    def test_verify_fragment_inline_chksum_succeed(self):
        pyeclib_drivers = self.get_pyeclib_testspec("inline_crc32")

        filesize = 1024 * 1024 * 3
        file_str = ''.join(random.choice(ascii_letters)
                           for i in range(filesize))
        file_bytes = file_str.encode('utf-8')

        for pyeclib_driver in pyeclib_drivers:
            fragments = pyeclib_driver.encode(file_bytes)

            fragment_metadata_list = []

            for fragment in fragments:
                fragment_metadata_list.append(
                    pyeclib_driver.get_metadata(fragment))

            expected_ret_value = {"status": 0}

            self.assertTrue(pyeclib_driver.verify_stripe_metadata(
                fragment_metadata_list) == expected_ret_value)

    def test_get_segment_byterange_info(self):
        pyeclib_drivers = self.get_pyeclib_testspec()
        if not pyeclib_drivers:
            return

        file_size = 1024 * 1024
        segment_size = 3 * 1024

        ranges = [
            (0, 1),
            (1, 12),
            (10, 1000),
            (0, segment_size - 1),
            (1, segment_size + 1),
            (segment_size - 1, 2 * segment_size)]

        expected_results = {}

        expected_results[(0, 1)] = {0: (0, 1)}
        expected_results[(1, 12)] = {0: (1, 12)}
        expected_results[(10, 1000)] = {0: (10, 1000)}
        expected_results[(0, segment_size - 1)] = {0: (0, segment_size - 1)}
        expected_results[(1, segment_size + 1)
                         ] = {0: (1, segment_size - 1), 1: (0, 1)}
        expected_results[
            (segment_size - 1, 2 * segment_size)] = {
            0: (segment_size - 1, segment_size - 1),
            1: (0, segment_size - 1),
            2: (0, 0)}

        results = pyeclib_drivers[0].get_segment_info_byterange(
            ranges, file_size, segment_size)

        for exp_result_key in expected_results:
            self.assertIn(exp_result_key, results)
            self.assertTrue(
                len(results[exp_result_key]) ==
                len(expected_results[exp_result_key]))
            exp_result_map = expected_results[exp_result_key]
            for segment_key in exp_result_map:
                self.assertIn(segment_key, results[exp_result_key])
                self.assertTrue(
                    results[exp_result_key][segment_key] ==
                    expected_results[exp_result_key][segment_key])

    def test_get_segment_info(self):
        pyeclib_drivers = self.get_pyeclib_testspec()

        file_sizes = [
            1024 * 1024,
            2 * 1024 * 1024,
            10 * 1024 * 1024,
            10 * 1024 * 1024 + 7]
        segment_sizes = [3 * 1024, 1024 * 1024]
        segment_strings = {}

        #
        # Generate some test segments for each segment size.
        # Use 2 * segment size, because last segment may be
        # greater than segment_size
        #
        char_set = ascii_uppercase + digits
        for segment_size in segment_sizes:
            segment_strings[segment_size] = ''.join(
                random.choice(char_set) for i in range(segment_size * 2))

        for pyeclib_driver in pyeclib_drivers:
            for file_size in file_sizes:
                for segment_size in segment_sizes:
                    #
                    # Compute the segment info
                    #
                    segment_info = pyeclib_driver.get_segment_info(
                        file_size,
                        segment_size)

                    num_segments = segment_info['num_segments']
                    segment_size = segment_info['segment_size']
                    fragment_size = segment_info['fragment_size']
                    last_segment_size = segment_info['last_segment_size']
                    last_fragment_size = segment_info['last_fragment_size']

                    computed_file_size = (
                        (num_segments - 1) * segment_size) + last_segment_size

                    #
                    # Verify that the segment sizes add up
                    #
                    self.assertTrue(computed_file_size == file_size)

                    encoded_fragments = pyeclib_driver.encode(
                        (segment_strings[segment_size][
                            :segment_size]).encode('utf-8'))

                    #
                    # Verify the fragment size
                    #
                    self.assertTrue(fragment_size == len(encoded_fragments[0]))

                    if last_segment_size > 0:
                        encoded_fragments = pyeclib_driver.encode(
                            (segment_strings[segment_size][
                                :last_segment_size]).encode('utf-8'))

                        #
                        # Verify the last fragment size, if there is one
                        #
                        self.assertTrue(
                            last_fragment_size == len(
                                encoded_fragments[0]))

    def test_greedy_decode_reconstruct_combination(self):
        # the testing spec defined at get_pyeclib_testspec() method
        # and if you want to test either other parameters or backends,
        # you can add the spec you want to test there.
        pyeclib_drivers = self.get_pyeclib_testspec()
        orig_data = os.urandom(1024 ** 2)
        for pyeclib_driver in pyeclib_drivers:
            encoded = pyeclib_driver.encode(orig_data)
            # make all fragment like (index, frag_data) format to feed
            # to combinations
            frags = [(i, frag) for i, frag in enumerate(encoded)]
            num_frags = pyeclib_driver.k + pyeclib_driver.m

            if pyeclib_driver.ec_type == PyECLib_EC_Types.flat_xor_hd:
                # flat_xord_hd is guaranteed to work with 2 or 3 failures
                tolerable_failures = pyeclib_driver.hd - 1
            else:
                # ... while others can tolerate more
                tolerable_failures = pyeclib_driver.m

            for check_frags_tuples in combinations(
                    frags, num_frags - tolerable_failures):
                # extract check_frags_tuples from [(index, data bytes), ...]
                # to [index, index, ...] and [data bytes, data bytes, ...]
                indexes, check_frags = zip(*check_frags_tuples)
                decoded = pyeclib_driver.decode(check_frags)
                self.assertEqual(
                    orig_data, decoded,
                    "assertion fail in decode %s from:%s" %
                    (pyeclib_driver, indexes))
                holes = [index for index in range(num_frags) if
                         index not in indexes]
                for hole in holes:
                    reconed = pyeclib_driver.reconstruct(
                        check_frags, [hole])[0]
                    self.assertEqual(
                        frags[hole][1], reconed,
                        "assertion fail in reconstruct %s target:%s "
                        "from:%s" % (pyeclib_driver, hole, indexes))

    def test_rs(self):
        pyeclib_drivers = self.get_pyeclib_testspec()

        for pyeclib_driver in pyeclib_drivers:
            for file_size in self.file_sizes:
                tmp_file = self.files[file_size]
                tmp_file.seek(0)
                whole_file_bytes = tmp_file.read()

                encode_input = whole_file_bytes
                orig_fragments = pyeclib_driver.encode(encode_input)

                for iter in range(self.num_iterations):
                    num_missing = 2
                    idxs_to_remove = random.sample(range(
                        pyeclib_driver.k + pyeclib_driver.m), num_missing)
                    fragments = orig_fragments[:]

                    # Reverse sort the list, so we can always
                    # remove from the original index
                    idxs_to_remove.sort(reverse=True)
                    for idx in idxs_to_remove:
                        fragments.pop(idx)

                    #
                    # Test decoder
                    #
                    decoded_string = pyeclib_driver.decode(fragments)

                    self.assertTrue(encode_input == decoded_string)

                    #
                    # Test reconstructor
                    #
                    reconstructed_fragments = pyeclib_driver.reconstruct(
                        fragments,
                        idxs_to_remove)
                    self.assertEqual(len(reconstructed_fragments),
                                     len(idxs_to_remove))
                    for idx, frag_data in zip(idxs_to_remove,
                                              reconstructed_fragments):
                        self.assertEqual(
                            frag_data, orig_fragments[idx],
                            'Failed to reconstruct fragment %d!' % idx)

                    #
                    # Test decode with integrity checks
                    #
                    first_fragment_to_corrupt = random.randint(
                        0, len(fragments))
                    num_to_corrupt = min(len(fragments), pyeclib_driver.m + 1)
                    fragments_to_corrupt = [
                        (first_fragment_to_corrupt + i) % len(fragments)
                        for i in range(num_to_corrupt)]

                    if StrictVersion(LIBERASURECODE_VERSION) < \
                            StrictVersion('1.2.0'):
                        # if liberasurecode is older than the version supports
                        # fragment integrity check, skip following test
                        continue
                    i = 0
                    for fragment in fragments:
                        if i in fragments_to_corrupt:
                            corrupted_fragment = (
                                "0" * len(fragment)).encode('utf-8')
                            fragments[i] = corrupted_fragment
                        i += 1

                    self.assertRaises(ECInvalidFragmentMetadata,
                                      pyeclib_driver.decode,
                                      fragments, force_metadata_checks=True)

    def get_available_backend(self, k, m, ec_type, chksum_type="inline_crc32"):
        if ec_type[:11] == "flat_xor_hd":
            return ECDriver(k=k, m=m, ec_type="flat_xor_hd",
                            chksum_type=chksum_type)
        elif ec_type in VALID_EC_TYPES:
            return ECDriver(k=k, m=m, ec_type=ec_type, chksum_type=chksum_type)
        else:
            return None

    def test_liberasurecode_insufficient_frags_error(self):
        file_size = self.file_sizes[0]
        tmp_file = self.files[file_size]
        tmp_file.seek(0)
        whole_file_bytes = tmp_file.read()
        for ec_type in ['flat_xor_hd_3', 'liberasurecode_rs_vand']:
            if ec_type in VALID_EC_TYPES:
                pyeclib_driver = self.get_available_backend(
                    k=10, m=5, ec_type=ec_type)
                fragments = pyeclib_driver.encode(whole_file_bytes)
                self.assertRaises(ECInsufficientFragments,
                                  pyeclib_driver.reconstruct,
                                  [fragments[0]], [1, 2, 3, 4, 5, 6])

    def test_min_parity_fragments_needed(self):
        pyeclib_drivers = []
        for ec_type in ['flat_xor_hd_3', 'liberasurecode_rs_vand']:
            if ec_type in VALID_EC_TYPES:
                pyeclib_drivers.append(ECDriver(k=10, m=5, ec_type=ec_type))
                self.assertTrue(
                    pyeclib_drivers[0].min_parity_fragments_needed() == 1)

    def test_pyeclib_driver_repr_expression(self):
        pyeclib_drivers = self.get_pyeclib_testspec()
        for driver in pyeclib_drivers:
            if driver.ec_type.name == 'flat_xor_hd':
                name = 'flat_xor_hd_%s' % driver.hd
            else:
                name = driver.ec_type.name

            self.assertEqual(
                "ECDriver(ec_type='%s', k=%s, m=%s)" %
                (name, driver.k, driver.m), repr(driver))

    def test_get_segment_info_memory_usage(self):
        for ec_driver in self.get_pyeclib_testspec():
            self._test_get_segment_info_memory_usage(ec_driver)

    def _test_get_segment_info_memory_usage(self, ec_driver):
        # 1. Preapre the expected memory allocation
        ec_driver.get_segment_info(1024 * 1024, 1024 * 1024)
        loop_range = range(1000)

        # 2. Get current memory usage
        usage = resource.getrusage(resource.RUSAGE_SELF)[2]

        # 3. Loop to call get_segment_info
        for x in loop_range:
            ec_driver.get_segment_info(1024 * 1024, 1024 * 1024)

        # 4. memory usage shoudln't be increased
        self.assertEqual(usage, resource.getrusage(resource.RUSAGE_SELF)[2],
                         'Memory usage is increased unexpectedly %s - %s' %
                         (usage, resource.getrusage(resource.RUSAGE_SELF)[2]))

    def test_get_metadata_memory_usage(self):
        for ec_driver in self.get_pyeclib_testspec():
            self._test_get_metadata_memory_usage(ec_driver)

    def _test_get_metadata_memory_usage(self, ec_driver):
        # 1. Prepare the expected memory allocation
        encoded = ec_driver.encode(b'aaa')
        ec_driver.get_metadata(encoded[0], formatted=True)
        loop_range = range(400000)

        # 2. Get current memory usage
        baseline_usage = resource.getrusage(resource.RUSAGE_SELF)[2]

        # 3. Loop to call get_metadata
        for x in loop_range:
            ec_driver.get_metadata(encoded[0], formatted=True)

        # 4. memory usage shouldn't increase
        new_usage = resource.getrusage(resource.RUSAGE_SELF)[2]
        self.assertEqual(baseline_usage, new_usage,
                         'Memory usage is increased unexpectedly %s -> %s' %
                         (baseline_usage, new_usage))


class BackendsEnabledMetaclass(type):
    def __new__(meta, cls_name, cls_bases, cls_dict):
        for ec_type in ALL_EC_TYPES:
            def dummy(self, ec_type=ec_type):
                if ec_type not in VALID_EC_TYPES:
                    raise unittest.SkipTest
                if ec_type == 'shss':
                    k = 10
                    m = 4
                elif ec_type == 'libphazr':
                    k = 4
                    m = 4
                else:
                    k = 10
                    m = 5
                ECDriver(k=k, m=m, ec_type=ec_type)
            dummy.__name__ = 'test_%s_available' % ec_type
            cls_dict[dummy.__name__] = dummy
        return type.__new__(meta, cls_name, cls_bases, cls_dict)


class TestBackendsEnabled(six.with_metaclass(BackendsEnabledMetaclass,
                                             unittest.TestCase)):
    '''Based on TestPyECLibDriver.test_valid_algo above, but these tests
       should *always* either pass or skip.'''


if __name__ == '__main__':
    unittest.main()
