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

import sys
import types

#
# Generic ECDriverException
#


class ECDriverError(Exception):

    def __init__(self, error_str):
        self.error_str = error_str

    def __str__(self):
        return self.error_str


def import_class(import_str):
    """
    Returns a class from a string that specifies a module and/or class

    :param import_str: import path, e.g. 'httplib.HTTPConnection'
    :returns imported object
    :raises: ImportedError if the class does not exist or the path is invalid
    """
    (mod_str, separator, class_str) = import_str.rpartition('.')
    try:
        __import__(mod_str)
        return getattr(sys.modules[mod_str], class_str)
    except (ValueError, AttributeError) as e:
        raise ImportError('Class %s cannot be found (%)' %
                          (class_str,
                           traceback.format_exception(*sys.exc_info())))


def create_instance(import_str, *args, **kwargs):
    """
    Returns instance of class which imported by import path.
    :param import_str: import path of class
    :param \*args: indexed arguments for new instance
    :param \*\*kwargs: keyword arguments for new instance
    :returns: instance of imported class which instantiated with
    arguments *args and **kwargs
    """
    try:
        object_class = import_class(import_str)
    except Exception as e:
        raise
    instance = object_class(*args, **kwargs)

    return instance


class ECDriver(object):

    def __init__(self, *args, **kwargs):
        self.k = -1
        self.m = -1
        self.w = -1
        for (key, value) in kwargs.items():
            if key == "k":
                self.k = int(value)
            if key == "m":
                self.m = int(value)
            if key == "w":
                self.w = int(value)

        self.library_import_str = kwargs.pop('library_import_str',
                                             'pyeclib.core.ECPyECLibDriver')

        if self.k < 0:
            raise ECDriverError(
                "Number of data fragments (k) was not specified "
                "and is a required argument!")
        if self.m < 0:
            raise ECDriverError(
                "Number of parity fragments (m) was not specified "
                "and is a required argument!")

        #
        # We require keyword arguments to prevent ambiguity between EC libs
        #
        self.ec_lib_reference = create_instance(
            self.library_import_str,
            *args,
            **kwargs)

        #
        # Verify that the imported library implements the required functions
        #
        required_methods = {
            'decode': 0,
            'encode': 0,
            'reconstruct': 0,
            'fragments_needed': 0,
            'min_parity_fragments_needed': 0,
            'get_metadata': 0,
            'verify_stripe_metadata': 0,
            'get_segment_info': 0
        }

        for attr in dir(self.ec_lib_reference):
            if hasattr(getattr(self.ec_lib_reference, attr), "__call__"):
                required_methods[attr] = 1

        not_implemented_str = ""
        for (method, is_implemented) in required_methods.items():
            if is_implemented == 0:
                not_implemented_str += method + " "

        if len(not_implemented_str) > 0:
            raise ECDriverError(
                "The following required methods are not implemented "
                "in %s: %s" % (self.library_import_str, not_implemented_str))

    def encode(self, data_bytes):
        """
        Encode an arbitrary-sized string
        :param data_bytes: the buffer to encode
        :returns: a list of buffers (first k entries are data and
                  the last m are parity)
        :raises: ECDriverError if there is an error during encoding
        """
        return self.ec_lib_reference.encode(data_bytes)

    def decode(self, fragment_payloads):
        """
        Decode a set of fragments into a buffer that represents the original
        buffer passed into encode().

        :param fragment_payloads: a list of buffers representing a subset of
                                  the list generated by encode()
        :returns: a buffer
        :raises: ECDriverError if there is an error during decoding
        """
        return self.ec_lib_reference.decode(fragment_payloads)

    def reconstruct(self, available_fragment_payloads,
                    missing_fragment_indexes):
        """
        Reconstruct a missing fragment from a subset of available fragments.

        :param available_fragment_payloads: a list of buffers representing
                                            a subset of the list generated
                                            by encode()
        :param missing_fragment_indexes: a list of integers representing
                                         the indexes of the fragments to be
                                         reconstructed.
        :param destination_index: the index of the element to reconstruct
        :returns: a list of buffers (ordered by fragment index) containing
                  the reconstructed payload associated with the indexes
                  provided in missing_fragment_indexes
        :raises: ECDriverError if there is an error during decoding or there
                 are not sufficient fragments to decode
        """
        return self.ec_lib_reference.reconstruct(
            available_fragment_payloads, missing_fragment_indexes)

    def fragments_needed(self, missing_fragment_indexes):
        """
        Determine which fragments are needed to reconstruct some subset of
        missing fragments.

        :param missing_fragment_indexes: a list of integers representing the
                                         indexes of the fragments to be
                                         reconstructed.
        :returns: a list of lists containing fragment indexes.  Each sub-list
                  contains a combination of fragments that can be used to
                  reconstruct the missing fragments.
        :raises: ECDriverError if there is an error during decoding or there
                 are not sufficient fragments to decode
        """
        return self.ec_lib_reference.fragments_needed(missing_fragment_indexes)

    def min_parity_fragments_needed(self):
        return self.ec_lib_reference.min_parity_fragments_needed()

    def get_metadata(self, fragment):
        """
        Get opaque metadata for a fragment.  The metadata is opaque to the
        client, but meaningful to the underlying library.  It is used to verify
        stripes in verify_stripe_metadata().

        :param fragment: a buffer representing a single fragment generated by
                         the encode() function.
        :returns: an opaque buffer to be passed into verify_stripe_metadata()
        :raises: ECDriverError if there was a problem getting the metadata.
        """
        return self.ec_lib_reference.get_metadata(fragment)

    def verify_stripe_metadata(self, fragment_metadata_list):
        """
        Verify a subset of fragments generated by encode()

        :param fragment_metadata_list: a list of buffers representing the
                                       metadata from a subset of the framgments
                                       generated by encode().
        :returns: 'None' if the metadata is consistent.
                  a list of fragment indexes corresponding to inconsistent
                  fragments
        :raises: ECDriverError if there was a problem verifying the metadata

        """
        return self.ec_lib_reference.verify_stripe_metadata(
            fragment_metadata_list)

    def get_segment_info(self, data_len, segment_size):
        """
        Get segmentation info for a given data length and
        segment size.

        Semment info returns a dict with the following keys:

        segment_size: size of the payload to give to encode()
        last_segment_size: size of the payload to give to encode()
        fragment_size: the fragment size returned by encode()
        last_fragment_size: the fragment size returned by encode()
        num_segments: number of segments

        This allows the caller to prepare requests
        when segmenting a data stream to be EC'd.

        Since the data length will rarely be aligned
        to the segment size, the last segment will be
        a different size than the others.

        There are restrictions on the length given to encode(),
        so calling this before encode is highly recommended when
        segmenting a data stream.
        """
        return self.ec_lib_reference.get_segment_info(data_len, segment_size)
