PyEClib
-------

This library provides a simple Python interface for implementing erasure codes
and is known to work with Python v2.6, 2.7 and 3.x.

To obtain the best possible performance, the library utilizes liberasurecode,
which is a C based erasure code library.  Please let us know if you have any
issues building or installing (email: kmgreen2@gmail.com or tusharsg@gmail.com).

PyECLib supports a variety of Erasure Coding backends including the standard Reed
Soloman implementations provided by Jerasure [1], liberasurecode [3] and Intel
ISA-L [4].  It also provides support for a flat XOR-based encoder and decoder
(part of liberasurecode) - a class of HD Combination Codes based on "Flat
XOR-based erasure codes in storage systems: Constructions, efficient recovery,
and tradeoffs" in IEEE MSST 2010[2]).  These codes are well-suited to archival
use-cases, have a simple construction and require a minimum number of
participating disks during single-disk reconstruction (think XOR-based LRC code).

Examples of using PyECLib are provided in the "tools" directory:

  Command-line encoder::

      tools/pyeclib_encode.py

  Command-line decoder::

      tools/pyeclib_decode.py

  Utility to determine what is needed to reconstruct missing fragments::

      tools/pyeclib_fragments_needed.py

  A configuration utility to help compare available EC schemes in terms of
  performance and redundancy::

      tools/pyeclib_conf_tool.py

PyEClib initialization::

    ec_driver = ECDriver(k=<num_encoded_data_fragments>,
                         m=<num_encoded_parity_fragments>,
                         ec_type=<ec_scheme>))

Supported ``ec_type`` values:

  * ``liberasurecode_rs_vand`` => Vandermonde Reed-Solomon encoding, software-only backend implemented by liberasurecode [3]
  * ``jerasure_rs_vand`` => Vandermonde Reed-Solomon encoding, based on Jerasure [1]
  * ``jerasure_rs_cauchy`` => Cauchy Reed-Solomon encoding (Jerasure variant), based on Jerasure [1]
  * ``flat_xor_hd_3``, ``flat_xor_hd_4`` => Flat-XOR based HD combination codes, liberasurecode [3]
  * ``isa_l_rs_vand`` => Intel Storage Acceleration Library (ISA-L) - SIMD accelerated Erasure Coding backends [4]
  * ``isa_l_rs_cauchy`` => Cauchy Reed-Solomon encoding (ISA-L variant) [4]
  * ``shss`` => NTT Lab Japan's Erasure Coding Library [5]
  * ``libphazr`` => Phazr.IO's erasure code library with built-in privacy [6]


The Python API supports the following functions:

- EC Encode

  Encode N bytes of a data object into k (data) + m (parity) fragments::

      def encode(self, data_bytes)

      input:   data_bytes - input data object (bytes)
      returns: list of fragments (bytes)
      throws:
        ECBackendInstanceNotAvailable - if the backend library cannot be found
        ECBackendNotSupported - if the backend is not supported by PyECLib (see ec_types above)
        ECInvalidParameter - if invalid parameters were provided
        ECOutOfMemory - if the process has run out of memory
        ECDriverError - if an unknown error occurs

- EC Decode

  Decode between k and k+m fragments into original object::

      def decode(self, fragment_payloads)

      input:   list of fragment_payloads (bytes)
      returns: decoded object (bytes)
      throws:
        ECBackendInstanceNotAvailable - if the backend library cannot be found
        ECBackendNotSupported - if the backend is not supported by PyECLib (see ec_types above)
        ECInvalidParameter - if invalid parameters were provided
        ECOutOfMemory - if the process has run out of memory
        ECInsufficientFragments - if an insufficient set of fragments has been provided (e.g. not enough)
        ECInvalidFragmentMetadata - if the fragment headers appear to be corrupted
        ECDriverError - if an unknown error occurs


*Note*: ``bytes`` is a synonym to ``str`` in Python 2.6, 2.7.
In Python 3.x, ``bytes`` and ``str`` types are non-interchangeable and care
needs to be taken when handling input to and output from the ``encode()`` and
``decode()`` routines.


- EC Reconstruct

  Reconstruct "missing_fragment_indexes" using "available_fragment_payloads"::

      def reconstruct(self, available_fragment_payloads, missing_fragment_indexes)

      input: available_fragment_payloads - list of fragment payloads
      input: missing_fragment_indexes - list of indexes to reconstruct
      output: list of reconstructed fragments corresponding to missing_fragment_indexes
      throws:
        ECBackendInstanceNotAvailable - if the backend library cannot be found
        ECBackendNotSupported - if the backend is not supported by PyECLib (see ec_types above)
        ECInvalidParameter - if invalid parameters were provided
        ECOutOfMemory - if the process has run out of memory
        ECInsufficientFragments - if an insufficient set of fragments has been provided (e.g. not enough)
        ECInvalidFragmentMetadata - if the fragment headers appear to be corrupted
        ECDriverError - if an unknown error occurs


- Minimum parity fragments needed for durability gurantees::

      def min_parity_fragments_needed(self)

      NOTE: Currently hard-coded to 1, so this can only be trusted for MDS codes, such as
            Reed-Solomon.

      output: minimum number of additional fragments needed to be synchronously written to tolerate
              the loss of any one fragment (similar guarantees to 2 out of 3 with 3x replication)
      throws:
        ECBackendInstanceNotAvailable - if the backend library cannot be found
        ECBackendNotSupported - if the backend is not supported by PyECLib (see ec_types above)
        ECInvalidParameter - if invalid parameters were provided
        ECOutOfMemory - if the process has run out of memory
        ECDriverError - if an unknown error occurs


- Fragments needed for EC Reconstruct

  Return the indexes of fragments needed to reconstruct "missing_fragment_indexes"::

      def fragments_needed(self, missing_fragment_indexes)

      input: list of missing_fragment_indexes
      output: list of fragments needed to reconstruct fragments listed in missing_fragment_indexes
      throws:
        ECBackendInstanceNotAvailable - if the backend library cannot be found
        ECBackendNotSupported - if the backend is not supported by PyECLib (see ec_types above)
        ECInvalidParameter - if invalid parameters were provided
        ECOutOfMemory - if the process has run out of memory
        ECDriverError - if an unknown error occurs


- Get EC Metadata

  Return an opaque header known by the underlying library or a formatted header (Python dict)::

      def get_metadata(self, fragment, formatted = 0)

      input: raw fragment payload
      input: boolean specifying if returned header is opaque buffer or formatted string
      output: fragment header (opaque or formatted)
      throws:
        ECBackendInstanceNotAvailable - if the backend library cannot be found
        ECBackendNotSupported - if the backend is not supported by PyECLib (see ec_types above)
        ECInvalidParameter - if invalid parameters were provided
        ECOutOfMemory - if the process has run out of memory
        ECDriverError - if an unknown error occurs

- Verify EC Stripe Consistency

  Use opaque buffers from get_metadata() to verify a the consistency of a stripe::

      def verify_stripe_metadata(self, fragment_metadata_list)

      intput: list of opaque fragment headers
      output: formatted string containing the 'status' (0 is success) and 'reason' if verification fails
      throws:
        ECBackendInstanceNotAvailable - if the backend library cannot be found
        ECBackendNotSupported - if the backend is not supported by PyECLib (see ec_types above)
        ECInvalidParameter - if invalid parameters were provided
        ECOutOfMemory - if the process has run out of memory
        ECDriverError - if an unknown error occurs


- Get EC Segment Info

  Return a dict with the keys - segment_size, last_segment_size, fragment_size, last_fragment_size and num_segments::

      def get_segment_info(self, data_len, segment_size)

      input: total data_len of the object to store
      input: target segment size used to segment the object into multiple EC stripes
      output: a dict with keys - segment_size, last_segment_size, fragment_size, last_fragment_size and num_segments
      throws:
        ECBackendInstanceNotAvailable - if the backend library cannot be found
        ECBackendNotSupported - if the backend is not supported by PyECLib (see ec_types above)
        ECInvalidParameter - if invalid parameters were provided
        ECOutOfMemory - if the process has run out of memory
        ECDriverError - if an unknown error occurs


- Get EC Segment Info given a list of ranges, data length and segment size::

      def get_segment_info_byterange(self, ranges, data_len, segment_size)

      input: byte ranges
      input: total data_len of the object to store
      input: target segment size used to segment the object into multiple EC stripes
      output: (see below)
      throws:
        ECBackendInstanceNotAvailable - if the backend library cannot be found
        ECBackendNotSupported - if the backend is not supported by PyECLib (see ec_types above)
        ECInvalidParameter - if invalid parameters were provided
        ECOutOfMemory - if the process has run out of memory
        ECDriverError - if an unknown error occurs

  Assume a range request is given for an object with segment size 3K and
  a 1 MB file::

      Ranges = (0, 1), (1, 12), (10, 1000), (0, segment_size-1),
               (1, segment_size+1), (segment_size-1, 2*segment_size)

  This will return a map keyed on the ranges, where there is a recipe
  given for each range::

      {
       (0, 1): {0: (0, 1)},
       (10, 1000): {0: (10, 1000)},
       (1, 12): {0: (1, 12)},
       (0, 3071): {0: (0, 3071)},
       (3071, 6144): {0: (3071, 3071), 1: (0, 3071), 2: (0, 0)},
       (1, 3073): {0: (1, 3071), 1: (0,0)}
      }


Quick Start

  Install pre-requisites::

    * Python 2.6, 2.7 or 3.x (including development packages), argparse, setuptools
    * liberasurecode v1.2.0 or greater [3]
    * Erasure code backend libraries, gf-complete and Jerasure [1],[2], ISA-L [4] etc

  An example for ubuntu to install dependency packages::

      $ sudo apt-get install build-essential python-dev python-pip liberasurecode-dev
      $ sudo pip install -U bindep -r test-requirements.txt

  If you want to confirm all dependency packages installed successfully, try::

      $ sudo bindep -f bindep.txt

  *Note*: currently liberasurecode-dev/liberasurecode-devel in package repo is older than v1.2.0

  Install PyECLib::

      $ sudo python setup.py install

  Run test suite included::

      $ ./.unittests

  If all of this works, then you should be good to go.  If not, send us an email!

  If the test suite fails because it cannot find any of the shared libraries,
  then you probably need to add /usr/local/lib to the path searched when loading
  libraries.  The best way to do this (on Linux) is to add '/usr/local/lib' to::

      /etc/ld.so.conf

  and then make sure to run::

      $ sudo ldconfig


References

 [1] Jerasure, C library that supports erasure coding in storage applications, http://jerasure.org

 [2] Greenan, Kevin M et al, "Flat XOR-based erasure codes in storage systems", http://www.kaymgee.com/Kevin_Greenan/Publications_files/greenan-msst10.pdf

 [3] liberasurecode, C API abstraction layer for erasure coding backends, https://github.com/openstack/liberasurecode

 [4] Intel(R) Storage Acceleration Library (Open Source Version), https://01.org/intel%C2%AE-storage-acceleration-library-open-source-version

 [5] Kota Tsuyuzaki <tsuyuzaki.kota@lab.ntt.co.jp>, "NTT SHSS Erasure Coding backend"

 [6] Jim Cheung <support@phazr.io>, "Phazr.IO libphazr erasure code backend with built-in privacy"
