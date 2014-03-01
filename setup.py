#!/usr/bin/env python

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


try:
    from setuptools import setup
except ImportError:
    from distribute_setup import use_setuptools
    use_setuptools()
    from setuptools import setup

from distutils.command.build import build as _build
from distutils.command.clean import clean as _clean
from setuptools.command.install import install as _install

import os
import platform
import sys

from setuptools import Extension

#
# Fuck Apple and their universal binaries!
# I am not supporting powerpc, so ignoring it
#
autoconf_arguments = ""

platform_str = platform.platform()
if platform_str.find("Darwin") > -1:
    if platform_str.find("x86_64") > -1 and platform_str.find("i386") > -1:
        autoconf_arguments = 'CC="gcc -arch i386 -arch x86_64" CPP="gcc -E"'

try:
    python_library_name = "python%d.%d" % (sys.version_info.major,
                                           sys.version_info.minor)
except Exception:
    python_library_name = "python%d.%d" % (sys.version_info[0],
                                           sys.version_info[1])


def _read_file_as_str(name):
    with open(name, 'r') as f:
        s = f.readline().strip()
    return s

c_eclib_dir = "c_eclib-0.2"


# Build CPPFLAGS, LDFLAGS, LIBS
def _construct_jerasure_buildenv():
    _cppflags = _read_file_as_str("%s/._cppflags" % c_eclib_dir)
    _ldflags = _read_file_as_str("%s/._ldflags" % c_eclib_dir)
    _libs = _read_file_as_str("%s/._libs" % c_eclib_dir)
    return _cppflags, _ldflags, _libs

#
# TODO(packaging): Figure out why Pypi is chaning the perms of the files when
# unpacking...  Is the umask set to 022 or something?
#
global cppflags
global ldflags
global libs


def _pre_build(dir):
    ret = os.system('(cd %s && chmod 755 build.sh && \
                      ./build.sh)' % c_eclib_dir)
    if ret != 0:
        sys.exit(ret)

    cppflags, ldflags, libs = _construct_jerasure_buildenv()
    os.environ['CPPFLAGS'] = cppflags
    os.environ['LDFLAGS'] = ldflags
    os.environ['LIBS'] = libs


class build(_build):
    def run(self):
        self.execute(_pre_build, (self.build_lib,),
                     msg="Running pre build task(s)")
        _build.run(self)


class clean(_clean):
    def run(self):
        os.system('(cd %s && chmod 755 clean.sh && \
                   ./clean.sh)' % c_eclib_dir)
        _clean.run(self)


class install(_install):
    def run(self):
        install_lib = self.distribution.get_command_obj('install_lib')
        install_scripts = self.distribution.get_command_obj('install_scripts')
        install_cmd = self.distribution.get_command_obj('install')
        for cmd in (install_lib, install_scripts, install_cmd):
            cmd.ensure_finalized()

        # ensure that the paths are absolute so we don't get lost
        opts = {'prefix': install_cmd.prefix,
                'install-dir': install_lib.install_dir}
        for optname, value in opts.items():
            if value is not None:
                opts[optname] = os.path.abspath(value)

        ret = os.system('(cd %s && chmod 755 install.sh && \
                          ./install.sh %s)' %
                        (c_eclib_dir, opts['prefix']))
        if ret != 0:
            sys.exit(ret)

        _install.run(self)


module = Extension('pyeclib_c',
                   define_macros=[('MAJOR VERSION', '0'),
                                  ('MINOR VERSION', '2')],
                   include_dirs=['/usr/include/%s' % python_library_name,
                                 '/usr/local/include',
                                 '/usr/include',
                                 'src/c/pyeclib_c',
                                 '%s/include' % c_eclib_dir,
                                 '/usr/local/include'],
                   library_dirs=['/usr/lib', '/usr/local/lib'],
                   libraries=['Jerasure', 'Xorcode', 'alg_sig'],
                   # The extra arguments are for debugging
                   # extra_compile_args=['-g', '-O0'],
                   sources=['src/c/pyeclib_c/pyeclib_c.c'])

setup(name='PyECLib',
      version='0.2.2',
      author='Kevin Greenan',
      author_email='kmgreen2@gmail.com',
      maintainer='Kevin Greenan and Tushar Gohad',
      maintainer_email='kmgreen2@gmail.com, tusharsg@gmail.com',
      url='https://bitbucket.org/kmgreen2/pyeclib',
      description='This library provides a simple Python interface for \
                   implementing erasure codes.  To obtain the best possible \
                   performance, the underlying erasure code algorithms are \
                   written in C.',
      platforms='Linux',
      license='BSD',
      ext_modules=[module],
      packages=['pyeclib'],
      package_dir={'pyeclib': 'src/python/pyeclib'},
      cmdclass={'build': build, 'install': install, 'clean': clean},
      py_modules=['pyeclib.ec_iface', 'pyeclib.core'])
