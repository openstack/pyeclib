
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

import pyeclib_c
import math

#
# Generic ECPyECLibException
#


class ECPyECLibException(Exception):

    def __init__(self, error_str):
        self.error_str = error_str

    def __str__(self):
        return self.error_str


class ECPyECLibDriver(object):

    def __init__(self, k, m, ec_type, chksum_type="none"):
        self.ec_rs_vand = "jerasure_rs_vand"
        self.ec_rs_cauchy_orig = "jerasure_rs_cauchy_orig"
        self.ec_flat_xor_3 = "flat_xor_3"
        self.ec_flat_xor_4 = "flat_xor_4"
        self.chksum_none = "none"
        self.chksum_inline = "inline"
        self.chksum_algsig = "algsig"
        self.ec_types = [
            self.ec_rs_vand,
            self.ec_rs_cauchy_orig,
            self.ec_flat_xor_3,
            self.ec_flat_xor_4]
        self.chksum_types = [
            self.chksum_none,
            self.chksum_inline,
            self.chksum_algsig]
        self.ec_rs_vand_best_w = 16
        self.ec_default_w = 32
        self.ec_rs_cauchy_best_w = 4
        self.k = k
        self.m = m

        #
        # Override the default wordsize (w) for Reed-Solomon, if specified
        #
        if ec_type[:len(self.ec_rs_vand)] == self.ec_rs_vand \
                and len(ec_type) > len(self.ec_rs_vand):
            type_ary = ec_type.split("_")
            if len(type_ary) != 3:
                raise ECPyECLibException(
                    "%s is not a valid EC type for PyECLib!" %
                    ec_type)
            self.ec_rs_vand_best_w = int(type_ary[2])
            ec_type = self.ec_rs_vand

        #
        # Override the default wordsize (w) for Cauchy, if specified
        #
        if ec_type[:len(self.ec_rs_cauchy_orig)] == self.ec_rs_cauchy_orig \
                and len(ec_type) > len(self.ec_rs_cauchy_orig):
            type_ary = ec_type.split("_")
            if len(type_ary) != 4:
                raise ECPyECLibException(
                    "%s is not a valid EC type for PyECLib!" %
                    ec_type)
            self.ec_rs_cauchy_best_w = int(type_ary[3])
            ec_type = self.ec_rs_cauchy_orig

        if ec_type in self.ec_types:
            self.ec_type = ec_type
        else:
            raise ECPyECLibException("%s is not a valid EC type for PyECLib!")

        if chksum_type in self.chksum_types:
            self.chksum_type = chksum_type
        else:
            raise ECPyECLibException(
                "%s is not a valid checksum type for PyECLib!")

        if self.ec_type == self.ec_rs_vand:
            self.w = self.ec_rs_vand_best_w
            self.hd = self.m + 1
        elif self.ec_type == self.ec_rs_cauchy_orig:
            self.w = self.ec_rs_cauchy_best_w
            self.hd = self.m + 1
        elif self.ec_type == self.ec_flat_xor_3:
            self.w = self.ec_default_w
            self.hd = 3
        elif self.ec_type == self.ec_flat_xor_4:
            self.w = self.ec_default_w
            self.hd = 4
        else:
            self.w = self.ec_default_w

        self.inline_chksum = 0
        self.algsig_chksum = 0
        if self.chksum_type == self.chksum_inline:
            self.inline_chksum = 1
            self.algsig_chksum = 0
        elif self.chksum_type == self.chksum_algsig:
            self.inline_chksum = 0
            self.algsig_chksum = 1

        self.handle = pyeclib_c.init(
            self.k,
            self.m,
            self.w,
            self.ec_type,
            self.inline_chksum,
            self.algsig_chksum)

    def encode(self, data_bytes):
        return pyeclib_c.encode(self.handle, data_bytes)

    def decode(self, fragment_payloads):
        try:
            ret_string = pyeclib_c.fragments_to_string(
                self.handle,
                fragment_payloads)
        except Exception as e:
            raise ECPyECLibException("Error in ECPyECLibDriver.decode")

        if ret_string is None:
            (data_frags,
             parity_frags,
             missing_idxs) = pyeclib_c.get_fragment_partition(
                self.handle, fragment_payloads)
            decoded_fragments = pyeclib_c.decode(
                self.handle, data_frags, parity_frags, missing_idxs,
                len(data_frags[0]))
            ret_string = pyeclib_c.fragments_to_string(
                self.handle,
                decoded_fragments)

        return ret_string

    def reconstruct(self, fragment_payloads, indexes_to_reconstruct):
        reconstructed_data = []

        # Reconstruct the data, then the parity
        # The parity cannot be reconstructed until
        # after all data is reconstructed
        indexes_to_reconstruct.sort()
        _indexes_to_reconstruct = indexes_to_reconstruct[:]

        while len(_indexes_to_reconstruct) > 0:
            index = _indexes_to_reconstruct.pop(0)
            (data_frags,
             parity_frags,
             missing_idxs) = pyeclib_c.get_fragment_partition(
                self.handle, fragment_payloads)
            reconstructed = pyeclib_c.reconstruct(
                self.handle, data_frags, parity_frags, missing_idxs,
                index, len(data_frags[0]))
            reconstructed_data.append(reconstructed)

        return reconstructed_data

    def fragments_needed(self, missing_fragment_indexes):
        return pyeclib_c.get_required_fragments(
            self.handle, missing_fragment_indexes)

    def min_parity_fragments_needed(self):
        """ FIXME - fix this to return a function of HD """
        return 1

    def get_metadata(self, fragment):
        return pyeclib_c.get_metadata(self.handle, fragment)

    def verify_stripe_metadata(self, fragment_metadata_list):
        return pyeclib_c.check_metadata(self.handle, fragment_metadata_list)

    def get_segment_info(self, data_len, segment_size):
        return pyeclib_c.get_segment_info(self.handle, data_len, segment_size)


class ECNullDriver(object):

    def __init__(self, k, m):
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

    def min_parity_fragments_needed(self):
        pass


#
# A striping-only driver for EC.  This is
# pretty much RAID 0.
#
class ECStripingDriver(object):

    def __init__(self, k, m):
        """
        Stripe an arbitrary-sized string into k fragments
        :param k: the number of data fragments to stripe
        :param m: the number of parity fragments to stripe
        :raises: ECPyECLibException if there is an error during encoding
        """
        self.k = k

        if m != 0:
            raise ECPyECLibException("This driver only supports m=0")

        self.m = m

    def encode(self, data_bytes):
        """
        Stripe an arbitrary-sized string into k fragments
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
        """
        Convert a k-fragment data stripe into a string
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
        """
        We cannot reconstruct a fragment using other fragments.  This means
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
        """
        By definition, all missing fragment indexes are needed to reconstruct,
        so just return the list handed to this function.
        :param missing_fragment_indexes: indexes of missing fragments
        :returns: missing_fragment_indexes
        """
        return missing_fragment_indexes

    def get_metadata(self, fragment):
        """
        This driver does not include fragment metadata, so return empty string
        :param fragment: a fragment
        :returns: empty string
        """
        return ''

    def verify_stripe_metadata(self, fragment_metadata_list):
        """
        This driver does not include fragment metadata, so return true
        :param fragment_metadata_list: a list of fragments
        :returns: True
        """
        return True

    def get_segment_info(self, data_len, segment_size):
        pass

    def min_parity_fragments_needed(self):
        pass
