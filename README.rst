PyEClib
-------

This library provides a simple Python interface for implementing erasure codes
and is known to work with Python v2.6, 2.7 and 3.x. To obtain the best possible
performance, the library utilizes liberasurecode, which is a C based erasure
code library.

PyECLib supports a variety of Erasure Coding backends including the standard
Reed-Solomon implementations provided by Jerasure [1], liberasurecode [3],
Intel's ISA-L [4] and Phazr.IO's libphazr.  It also provides support for a flat
XOR-based encoder and decoder (part of liberasurecode) - a class of HD
Combination Codes based on "Flat XOR-based erasure codes in storage systems:
Constructions, efficient recovery, and tradeoffs" in IEEE MSST 2010[2]).
These codes are well-suited to archival use-cases, have a simple construction
and require a minimum number of participating disks during single-disk
reconstruction (think XOR-based LRC code).

-----

Installation
============

Install pre-requisites:

* Python 2.6, 2.7 or 3.x (including development packages), argparse, setuptools
* liberasurecode v1.3.1 or greater [3]
* Erasure code backend libraries, gf-complete and Jerasure [1],[2], ISA-L [4], etc

Install dependencies:

Debian/Ubuntu hosts::

    $ sudo apt-get install build-essential python-dev python-pip liberasurecode-dev
    $ sudo pip install -U bindep -r test-requirements.txt

RHEL/CentOS hosts::
    
    $ sudo yum install -y redhat-lsb python2-pip python-devel liberasurecode-devel
    $ sudo pip install -U bindep -r test-requirements.txt
    $ tools/test-setup.sh

If you want to confirm all dependency packages installed successfully, try::

    $ sudo bindep -f bindep.txt

*Note*: Currently, for Ubuntu, liberasurecode-dev in package repo is older than v1.2.0.
For CentOS, make sure to install the latest Openstack Cloud SIG repo
to be able to install the latest available version of liberasurecode-devel.

Install PyECLib::

    $ sudo python setup.py install

Run test suite included::

    $ ./.unittests

If the test suite fails because it cannot find any of the shared libraries,
then you probably need to add /usr/local/lib to the path searched when loading
libraries.  The best way to do this (on Linux) is to add '/usr/local/lib' to::

    /etc/ld.so.conf

and then make sure to run::

    $ sudo ldconfig

-----

Getting started
===============

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

-----

Code Maintenance
================

This library is currently mainly maintained by the Openstack Swift community.
For questions or any other help, come ask in #openstack-swift on freenode.

-----

References
==========

[1] Jerasure, C library that supports erasure coding in storage applications, http://jerasure.org

[2] Greenan, Kevin M et al, "Flat XOR-based erasure codes in storage systems", http://www.kaymgee.com/Kevin_Greenan/Publications_files/greenan-msst10.pdf

[3] liberasurecode, C API abstraction layer for erasure coding backends, https://github.com/openstack/liberasurecode

[4] Intel(R) Storage Acceleration Library (Open Source Version), https://01.org/intel%C2%AE-storage-acceleration-library-open-source-version

[5] Kota Tsuyuzaki <tsuyuzaki.kota@lab.ntt.co.jp>, "NTT SHSS Erasure Coding backend"

[6] Jim Cheung <support@phazr.io>, "Phazr.IO libphazr erasure code backend with built-in privacy"
