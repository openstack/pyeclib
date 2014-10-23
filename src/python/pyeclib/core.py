# Copyright (c) 2013, 2014, Kevin Greenan (kmgreen2@gmail.com)
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

from ec_iface import PyECLib_FRAGHDRCHKSUM_Types
import math
import pyeclib_c
import sys

pyver = float('%s.%s' % sys.version_info[:2])


# Generic ECPyECLibException
class ECPyECLibException(Exception):

    def __init__(self, error_str):
        self.error_str = error_str

    def __str__(self):
        return self.error_str


class ECPyECLibDriver(object):

    def __init__(self, k, m, ec_type,
                 chksum_type=PyECLib_FRAGHDRCHKSUM_Types.none):
        self.k = k
        self.m = m
        self.ec_type = ec_type
        self.chksum_type = chksum_type
        hd = m

        self.inline_chksum = 0
        self.algsig_chksum = 0
        # crc32 is the only inline checksum type currently supported
        if self.chksum_type is PyECLib_FRAGHDRCHKSUM_Types.inline_crc32:
            self.inline_chksum = 1
        elif self.chksum_type is PyECLib_FRAGHDRCHKSUM_Types.algsig:
            self.algsig_chksum = 1

        name = self.ec_type.name

        if name == "flat_xor_hd":
          hd = 4 

        self.handle = pyeclib_c.init(
            self.k,
            self.m,
            ec_type.value, 
            hd,
            self.inline_chksum,
            self.algsig_chksum)

    def encode(self, data_bytes):
        return pyeclib_c.encode(self.handle, data_bytes)

    def _validate_and_return_fragment_size(self, fragments):
      if len(fragments) > 0 and len(fragments[0]) == 0:
        return -1
      fragment_len = len(fragments[0])

      for fragment in fragments[1:]:
        if len(fragment) != fragment_len:
          return -1

      return fragment_len

    def decode(self, fragment_payloads):
        fragment_len = self._validate_and_return_fragment_size(fragment_payloads)
        if fragment_len < 0:
            raise ECPyECLibException("Invalid fragment payload in ECPyECLibDriver.decode")

        if len(fragment_payloads) < self.k:
            raise ECPyECLibException("Not enough fragments given in ECPyECLibDriver.decode")

        return pyeclib_c.decode(self.handle, fragment_payloads, fragment_len)
            

    def reconstruct(self, fragment_payloads, indexes_to_reconstruct):
        fragment_len = self._validate_and_return_fragment_size(fragment_payloads)
        if fragment_len < 0:
            raise ECPyECLibException("Invalid fragment payload in ECPyECLibDriver.reconstruct")

        reconstructed_data = []
        _fragment_payloads = fragment_payloads[:]

        # Reconstruct the data, then the parity
        # The parity cannot be reconstructed until
        # after all data is reconstructed
        indexes_to_reconstruct.sort()
        _indexes_to_reconstruct = indexes_to_reconstruct[:]

        while len(_indexes_to_reconstruct) > 0:
            index = _indexes_to_reconstruct.pop(0)
            reconstructed = pyeclib_c.reconstruct(
                self.handle, _fragment_payloads, fragment_len, index)
            reconstructed_data.append(reconstructed)
            _fragment_payloads.append(reconstructed)

        return reconstructed_data

    def fragments_needed(self, reconstruct_indexes, exclude_indexes):
        return pyeclib_c.get_required_fragments(
            self.handle, reconstruct_indexes, exclude_indexes)

    def get_metadata(self, fragment):
        return pyeclib_c.get_metadata(self.handle, fragment)

    def verify_stripe_metadata(self, fragment_metadata_list):
        metadata_len = self._validate_and_return_fragment_size(fragment_payloads)
        if metadata_len < 0:
            raise ECPyECLibException("Invalid fragment payload in ECPyECLibDriver.verify_metadata")

        return pyeclib_c.check_metadata(self.handle, fragment_metadata_list, metadata_len)

    def get_segment_info(self, data_len, segment_size):
        return pyeclib_c.get_segment_info(self.handle, data_len, segment_size)


class ECNullDriver(object):

    def __init__(self, k, m, ec_type=None, chksum_type=None):
        self.k = k
        self.m = m

    def encode(self, data_bytes):
        pass

    def decode(self, fragment_payloads):
        pass

    def reconstruct(self, available_fragment_payloads,
                    missing_fragment_indexes):
        pass

    def fragments_needed(self, missing_fragment_indexes):
        pass

    def get_metadata(self, fragment):
        pass

    def verify_stripe_metadata(self, fragment_metadata_list):
        pass

    def get_segment_info(self, data_len, segment_size):
        pass


#
# A striping-only driver for EC.  This is
# pretty much RAID 0.
#
class ECStripingDriver(object):

    def __init__(self, k, m, ec_type=None, chksum_type=None):
        """Stripe an arbitrary-sized string into k fragments
        :param k: the number of data fragments to stripe
        :param m: the number of parity fragments to stripe
        :raises: ECPyECLibException if there is an error during encoding
        """
        self.k = k

        if m != 0:
            raise ECPyECLibException("This driver only supports m=0")

        self.m = m

    def encode(self, data_bytes):
        """Stripe an arbitrary-sized string into k fragments
        :param data_bytes: the buffer to encode
        :returns: a list of k buffers (data only)
        :raises: ECPyECLibException if there is an error during encoding
        """
        # Main fragment size
        fragment_size = math.ceil(len(data_bytes) / float(self.k))

        # Size of last fragment
        last_fragment_size = len(data_bytes) - (fragment_size * self.k - 1)

        fragments = []
        offset = 0
        for i in range(self.k - 1):
            fragments.append(data_bytes[offset:fragment_size])
            offset += fragment_size

        fragments.append(data_bytes[offset:last_fragment_size])

        return fragments

    def decode(self, fragment_payloads):
        """Convert a k-fragment data stripe into a string
        :param fragment_payloads: fragments (in order) to convert into a string
        :returns: a string containing the original data
        :raises: ECPyECLibException if there is an error during decoding
        """

        if len(fragment_payloads) != self.k:
            raise ECPyECLibException(
                "Decode requires %d fragments, %d fragments were given" %
                (len(fragment_payloads), self.k))

        ret_string = ''

        for fragment in fragment_payloads:
            ret_string += fragment

        return ret_string

    def reconstruct(self, available_fragment_payloads,
                    missing_fragment_indexes):
        """We cannot reconstruct a fragment using other fragments.  This means
        that reconstruction means all fragments must be specified, otherwise we
        cannot reconstruct and must raise an error.
        :param available_fragment_payloads: available fragments (in order)
        :param missing_fragment_indexes: indexes of missing fragments
        :returns: a string containing the original data
        :raises: ECPyECLibException if there is an error during reconstruction
        """
        if len(available_fragment_payloads) != self.k:
            raise ECPyECLibException(
                "Reconstruction requires %d fragments, %d fragments given" %
                (len(available_fragment_payloads), self.k))

        return available_fragment_payloads

    def fragments_needed(self, missing_fragment_indexes):
        """By definition, all missing fragment indexes are needed to
        reconstruct, so just return the list handed to this function.
        :param missing_fragment_indexes: indexes of missing fragments
        :returns: missing_fragment_indexes
        """
        return missing_fragment_indexes

    def get_metadata(self, fragment):
        """This driver does not include fragment metadata, so return empty
        string
        :param fragment: a fragment
        :returns: empty string
        """
        return ''

    def verify_stripe_metadata(self, fragment_metadata_list):
        """This driver does not include fragment metadata, so return true
        :param fragment_metadata_list: a list of fragments
        :returns: True
        """
        return True

    def get_segment_info(self, data_len, segment_size):
        pass
