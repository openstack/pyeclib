# Copyright (c) 2013-2014, Kevin Greenan (kmgreen2@gmail.com)
# Copyright (c) 2014, Tushar Gohad (tushar.gohad@intel.com)
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

from .enum import Enum
from .enum import unique
from .utils import create_instance
from .utils import positive_int_value
from pyeclib_c import get_liberasurecode_version

import logging
from logging.handlers import SysLogHandler
logger = logging.getLogger('pyeclib')
syslog_handler = SysLogHandler()
logger.addHandler(syslog_handler)


def check_backend_available(backend_name):
    try:
        from pyeclib_c import check_backend_available

        if backend_name.startswith('flat_xor_hd'):
            int_type = PyECLib_EC_Types.get_by_name('flat_xor_hd')
        else:
            int_type = PyECLib_EC_Types.get_by_name(backend_name)
        if not int_type:
            return False
        return check_backend_available(int_type.value)
    except ImportError:
        # check_backend_available has been supported since
        # liberasurecode>=1.2.0 so we need to define the func for older
        # liberasurecode version

        # select available k, m values
        if backend_name.startswith('flat_xor_hd'):
            k, m = (10, 5)
        else:
            k, m = (10, 4)
        try:
            ECDriver(ec_type=backend_name, k=k, m=m)
        except ECDriverError:
            return False
        return True


def PyECLibVersion(z, y, x):
    return (((z) << 16) + ((y) << 8) + (x))


PYECLIB_MAJOR = 1
PYECLIB_MINOR = 1
PYECLIB_REV = 2
PYECLIB_VERSION = PyECLibVersion(PYECLIB_MAJOR, PYECLIB_MINOR,
                                 PYECLIB_REV)


PYECLIB_MAX_DATA = 32
PYECLIB_MAX_PARITY = 32


@unique
class PyECLibEnum(Enum):

    def describe(self):
        # returns supported types
        return list(self)

    @classmethod
    def has_enum(cls, name):
        # returns True if name is a valid member of the enum
        try:
            cls.__getattr__(name)
        except AttributeError:
            return False
        return True

    @classmethod
    def get_by_name(cls, name):
        try:
            obj = cls.__getattr__(name)
        except AttributeError:
            return None
        return obj

    @classmethod
    def names(cls):
        return [name for name, value in cls.__members__.items()]

    @classmethod
    def values(cls):
        return [value for name, value in cls.__members__.items()]

    def __str__(self):
        return "%s: %d" % (self.name, self.value)


# Erasure Code backends supported as of this PyECLib API rev
class PyECLib_EC_Types(PyECLibEnum):
    # Note: the Enum start value defaults to 1 as the starting value and not 0
    # 0 is False in the boolean sense but enum members evaluate to True
    jerasure_rs_vand = 1
    jerasure_rs_cauchy = 2
    flat_xor_hd = 3
    isa_l_rs_vand = 4
    shss = 5
    liberasurecode_rs_vand = 6
    isa_l_rs_cauchy = 7
    libphazr = 8


# Output of Erasure (en)Coding process are data "fragments".  Fragment data
# integrity checks are provided by a checksum embedded in a header (prepend)
# for each fragment.

# The following Enum defines the schemes supported for fragment checksums.
# The checksum type is "none" unless specified.
class PyECLib_FRAGHDRCHKSUM_Types(PyECLibEnum):
    # Note: the Enum start value defaults to 1 as the starting value and not 0
    # 0 is False in the boolean sense but enum members evaluate to True
    none = 1
    inline_crc32 = 2


# Main ECDriver class
class ECDriver(object):
    '''A driver to encode, decode, and reconstruct erasure-coded data.'''

    def __init__(self, **kwargs):
        '''
        :param ec_type: the erasure coding type to use for this driver.
        :param k: number of data fragments to use. Required.
        :param m: number of parity fragments to use. Required.
        :param chksum_type:
        :param validate: default: False
        :param library_import_str: default: 'pyeclib.core.ECPyECLibDriver'

        You must provide either ``ec_type`` or ``library_import_str``;
        typically you just want to use ``ec_type``. See ALL_EC_TYPES for the
        list of all EC types supported by PyECLib, and VALID_EC_TYPES for the
        list of all EC types currently available on this system.
        '''
        self.k = -1
        self.m = -1
        self.hd = -1
        self.ec_type = None
        self.chksum_type = None
        self.validate = False

        for required in ('k', 'm'):
            if required not in kwargs:
                raise ECDriverError(
                    "Invalid Argument: %s is required" % required)

        if 'ec_type' not in kwargs and 'library_import_str' not in kwargs:
            raise ECDriverError(
                "Invalid Argument: either ec_type or library_import_str "
                "must be provided")

        for (key, value) in kwargs.items():
            if key == "k":
                try:
                    self.k = positive_int_value(value)
                except ValueError:
                    raise ECDriverError(
                        "Invalid number of data fragments (k)")
            elif key == "m":
                try:
                    self.m = positive_int_value(value)
                except ValueError:
                    raise ECDriverError(
                        "Invalid number of parity fragments (m)")
            elif key == "ec_type":
                if value in ["flat_xor_hd", "flat_xor_hd_3", "flat_xor_hd_4"]:
                    if value == "flat_xor_hd" or value == "flat_xor_hd_3":
                        self.hd = 3
                    elif value == "flat_xor_hd_4":
                        self.hd = 4
                    value = "flat_xor_hd"
                elif value == "libphazr":
                    self.hd = 1
                if PyECLib_EC_Types.has_enum(value):
                    self.ec_type = PyECLib_EC_Types.get_by_name(value)
                else:
                    raise ECBackendNotSupported(
                        "%s is not a valid EC type for PyECLib!" % value)
            elif key == "chksum_type":
                if PyECLib_FRAGHDRCHKSUM_Types.has_enum(value):
                    self.chksum_type = \
                        PyECLib_FRAGHDRCHKSUM_Types.get_by_name(value)
                else:
                    raise ECDriverError(
                        "%s is not a valid checksum type for PyECLib!" % value)
            elif key == "validate":
                # validate if the ec type is available (runtime check)
                self.validate = value

        if self.hd == -1:
            self.hd = self.m

        self.library_import_str = kwargs.pop('library_import_str',
                                             'pyeclib.core.ECPyECLibDriver')
        #
        # Instantiate EC backend driver
        #
        self.ec_lib_reference = create_instance(
            self.library_import_str,
            k=self.k,
            m=self.m,
            hd=self.hd,
            ec_type=self.ec_type,
            chksum_type=self.chksum_type,
            validate=int(self.validate)
        )

        #
        # Verify that the imported library implements the required functions
        #
        required_methods = [
            'decode',
            'encode',
            'reconstruct',
            'fragments_needed',
            'min_parity_fragments_needed',
            'get_metadata',
            'verify_stripe_metadata',
            'get_segment_info',
        ]

        missing_methods = ' '.join(
            method for method in required_methods
            if not callable(getattr(self.ec_lib_reference, method, None)))

        if missing_methods:
            raise ECDriverError(
                "The following required methods are not implemented "
                "in %s: %s" % (self.library_import_str, missing_methods))

    def __repr__(self):
        return '%s(ec_type=%r, k=%r, m=%r)' % (
            type(self).__name__,
            'flat_xor_hd_%s' % self.hd if self.ec_type.name == 'flat_xor_hd'
            else self.ec_type.name,
            self.k,
            self.m)

    def encode(self, data_bytes):
        """
        Encode an arbitrary-sized string
        :param data_bytes: the buffer to encode
        :returns: a list of buffers (first k entries are data and
                  the last m are parity)
        :raises: ECDriverError if there is an error during encoding
        """
        return self.ec_lib_reference.encode(data_bytes)

    def decode(self, fragment_payloads, ranges=None,
               force_metadata_checks=False):
        """
        Decode a set of fragments into a buffer that represents the original
        buffer passed into encode().

        :param fragment_payloads: a list of buffers representing a subset of
                                  the list generated by encode()
        :param ranges (optional): a list of byte ranges to return instead of
                                  the entire buffer
        :param force_metadata_checks (optional): validate collective integrity
                                  of the fragments before trying to decode
        :returns: a buffer
        :raises: ECDriverError if there is an error during decoding
        """
        return self.ec_lib_reference.decode(fragment_payloads, ranges,
                                            force_metadata_checks)

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
        :returns: a list of buffers (ordered by fragment index) containing
                  the reconstructed payload associated with the indexes
                  provided in missing_fragment_indexes
        :raises: ECDriverError if there is an error during decoding or there
                 are not sufficient fragments to decode
        """
        return self.ec_lib_reference.reconstruct(
            available_fragment_payloads, missing_fragment_indexes)

    def fragments_needed(self, reconstruction_indexes,
                         exclude_indexes=None):
        """
        Determine which fragments are needed to reconstruct some subset of
        missing fragments.

        :param reconstruction_indexes: a list of integers representing the
                                         indexes of the fragments to be
                                         reconstructed.
        :param exclude_indexes: a list of integers representing the
                                         indexes of the fragments to be
                                         excluded from the reconstruction
                                         equations.
        :returns: a list containing fragment indexes that can be used to
                  reconstruct the missing fragments.
        :raises: ECDriverError if there is an error during decoding or there
                 are not sufficient fragments to decode
        """
        if exclude_indexes is None:
            exclude_indexes = []
        return self.ec_lib_reference.fragments_needed(reconstruction_indexes,
                                                      exclude_indexes)

    def min_parity_fragments_needed(self):
        return self.ec_lib_reference.min_parity_fragments_needed()

    def get_metadata(self, fragment, formatted=0):
        """
        Get opaque metadata for a fragment.  The metadata is opaque to the
        client, but meaningful to the underlying library.  It is used to verify
        stripes in verify_stripe_metadata().

        :param fragment: a buffer representing a single fragment generated by
                         the encode() function.
        :returns: an opaque buffer to be passed into verify_stripe_metadata()
        :raises: ECDriverError if there was a problem getting the metadata.
        """
        return self.ec_lib_reference.get_metadata(fragment, formatted)

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

    #
    # Map of segment indexes with a list of tuples
    #
    def get_segment_info_byterange(self, ranges, data_len, segment_size):
        """
        Get segmentation info for a byterange request, given a data length and
        segment size.

        This will return a map-of-maps that represents a recipe describing
        the segments and ranges within each segment needed to satisfy a range
        request.

        Assume a range request is given for an object with segment size 3K and
        a 1 MB file:

        Ranges = (0, 1), (1, 12), (10, 1000), (0, segment_size-1),
                 (1, segment_size+1), (segment_size-1, 2*segment_size)

        This will return a map keyed on the ranges, where there is a recipe
        given for each range:

        {
         (0, 1): {0: (0, 1)},
         (10, 1000): {0: (10, 1000)},
         (1, 12): {0: (1, 12)},
         (0, 3071): {0: (0, 3071)},
         (3071, 6144): {0: (3071, 3071), 1: (0, 3071), 2: (0, 0)},
         (1, 3073): {0: (1, 3071), 1: (0,0)}
        }

        """

        segment_info = self.ec_lib_reference.get_segment_info(
            data_len, segment_size)

        segment_size = segment_info['segment_size']

        sorted_ranges = ranges[:]
        sorted_ranges.sort(key=lambda obj: obj[0])

        recipe = {}

        for r in ranges:
            segment_map = {}
            begin_off = r[0]
            end_off = r[1]
            begin_segment = begin_off // segment_size
            end_segment = end_off // segment_size

            if begin_segment == end_segment:
                begin_relative_off = begin_off % segment_size
                end_relative_off = end_off % segment_size
                segment_map[begin_segment] = (begin_relative_off,
                                              end_relative_off)
            else:
                begin_relative_off = begin_off % segment_size
                end_relative_off = end_off % segment_size

                segment_map[begin_segment] = (begin_relative_off,
                                              segment_size - 1)

                for middle_segment in range(begin_segment + 1, end_segment):
                    segment_map[middle_segment] = (0, segment_size - 1)

                segment_map[end_segment] = (0, end_relative_off)

            recipe[r] = segment_map

        return recipe


# PyECLib Exceptions

# Generic ECDriverException
class ECDriverError(Exception):
    def __init__(self, error):
        try:
            self.error_str = str(error)
        except Exception:
            self.error_str = 'Error retrieving the error message from %s' \
                % error.__class__.__name__

    def __str__(self):
        return self.error_str


# More specific exceptions, mapped to liberasurecode error codes

# Specified EC backend is not supported by PyECLib/liberasurecode
class ECBackendNotSupported(ECDriverError):
    pass


# Unsupported EC method
class ECMethodNotImplemented(ECDriverError):
    pass


# liberasurecode backend init error
class ECBackendInitializationError(ECDriverError):
    pass


# Specified backend instance is invalid/unavailable
class ECBackendInstanceNotAvailable(ECDriverError):
    pass


# Specified backend instance is busy
class ECBackendInstanceInUse(ECDriverError):
    pass


# Invalid parameter passed to a method
class ECInvalidParameter(ECDriverError):
    pass


# Invalid or incompatible fragment metadata
class ECInvalidFragmentMetadata(ECDriverError):
    pass


# Fragment checksum verification failed
class ECBadFragmentChecksum(ECDriverError):
    pass


# Insufficient fragments specified for decode or reconstruct operation
class ECInsufficientFragments(ECDriverError):
    pass


# Out of memory
class ECOutOfMemory(ECDriverError):
    pass


# PyECLib helper for "available" EC types
ALL_EC_TYPES = [
    'jerasure_rs_vand',
    'jerasure_rs_cauchy',
    'flat_xor_hd_3',
    'flat_xor_hd_4',
    'isa_l_rs_vand',
    'shss',
    'liberasurecode_rs_vand',
    'isa_l_rs_cauchy',
    'libphazr',
]


def _PyECLibValidECTypes():
    available_ec_types = []
    for _type in ALL_EC_TYPES:
        if check_backend_available(_type):
            available_ec_types.append(_type)
    return available_ec_types


VALID_EC_TYPES = _PyECLibValidECTypes()


def _liberasurecode_version():
    version_int = get_liberasurecode_version()
    version_hex_str = hex(version_int)
    version_hex_str = version_hex_str.lstrip('0x')
    major = str(int(version_hex_str[-6:-4]))
    minor = str(int(version_hex_str[-4:-2]))
    rev = str(int(version_hex_str[-2:]))
    version_str = '.'.join([major, minor, rev])

    # liberasurecode < 1.3.1 should be incompatible but
    # just warn until packagers build the required version
    # See https://bugs.launchpad.net/swift/+bug/1639691 in detail
    required_version = ((1 << 16) + (3 << 8) + 1)
    if version_int < required_version:
        logger.warning(
            'DEPRECATED WARNING: your liberasurecode '
            '%s will be deprecated in the near future because of the issue '
            'https://bugs.launchpad.net/swift/+bug/1639691; '
            'Please upgrade to >=1.3.1 and rebuild pyeclib to suppress '
            'this message' % version_str)
    return version_str


LIBERASURECODE_VERSION = _liberasurecode_version()
