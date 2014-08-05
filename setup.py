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

import os
import platform
import sys

from distutils.command.build import build as _build
from distutils.command.clean import clean as _clean
from distutils.sysconfig import EXEC_PREFIX as _exec_prefix
from distutils.sysconfig import get_python_lib
from distutils.sysconfig import get_python_inc

try:
    from setuptools import setup
except ImportError:
    from distribute_setup import use_setuptools
    use_setuptools()
    from setuptools import setup

from setuptools import Extension
from setuptools.command.install import install as _install

#
# Fuck Apple and their universal binaries!
# I am not supporting powerpc, so ignoring
# it
#
autoconf_arguments = "ac_none"

platform_str = platform.platform()
if platform_str.find("Darwin") > -1:
    if platform_str.find("x86_64") > -1 and platform_str.find("i386") > -1:
        autoconf_arguments = 'CC="gcc -arch i386 -arch x86_64" CPP="gcc -E"'

default_python_incdir = get_python_inc()
default_python_libdir = get_python_lib()
default_library_paths = [default_python_libdir,
                         ('%s/usr/local/lib' % _exec_prefix),
                         '/lib', '/usr/lib', '/usr/local/lib']

# build CPPFLAGS, LDFLAGS, LIBS
global cppflags
global ldflags
global libs

c_eclib_dir = "c_eclib-0.9"


def _construct_jerasure_buildenv():
    _cppflags = _read_file_as_str("%s/._cppflags" % c_eclib_dir)
    _ldflags = _read_file_as_str("%s/._ldflags" % c_eclib_dir)
    _libs = _read_file_as_str("%s/._libs" % c_eclib_dir)
    return _cppflags, _ldflags, _libs


# utility routine
def _read_file_as_str(name):
    with open(name, "rt") as f:
        s = f.readline().strip()
    return s


class build(_build):

    def run(self):
        ret = os.system('(cd %s && chmod 755 build-c_eclib.sh && \
                         ./build-c_eclib.sh "%s" %s)' %
                        (c_eclib_dir, autoconf_arguments, _exec_prefix))
        if ret != 0:
            sys.exit(ret)

        cppflags, ldflags, libs = _construct_jerasure_buildenv()
        os.environ['CPPFLAGS'] = cppflags
        os.environ['LDFLAGS'] = ldflags
        os.environ['LIBS'] = libs

        _build.run(self)


class clean(_clean):

    def run(self):
        ret = os.system('(cd %s && chmod 755 clean-c_eclib.sh && \
                          ./clean-c_eclib.sh)' % c_eclib_dir)
        if ret != 0:
            sys.exit(2)
        _clean.run(self)


class install(_install):

    def run(self):
        install_cmd = self.distribution.get_command_obj('install')
        install_lib = self.distribution.get_command_obj('install_lib')
        for cmd in (install_lib, install_cmd):
            cmd.ensure_finalized()

        # ensure that the paths are absolute so we don't get lost
        opts = {'exec_prefix': install_cmd.exec_prefix,
                'root': install_cmd.root}
        for optname, value in list(opts.items()):
            if value is not None:
                opts[optname] = os.path.abspath(value)

        prefix = opts['exec_prefix']
        root = opts['root']

        # prefer root for installdir
        if root is not None:
            installroot = root
        elif prefix is not None:
            installroot = prefix
        else:
            installroot = "/"

        # exception is "/usr"
        if installroot.startswith("/usr"):
            installroot = "/"

        ret = os.system('(cd %s && chmod 755 install-c_eclib.sh && \
                        ./install-c_eclib.sh %s)' %
                        (c_eclib_dir, installroot))
        if ret != 0:
            sys.exit(ret)

        default_library_paths.insert(0, "%s/usr/local/lib" % installroot)
        _install.run(self)

        #
        # Another Mac-ism...  If the libraries are installed
        # in a strange place, DYLD_LIRBARY_PATH needs to be
        # updated.
        #
        if platform_str.find("Darwin") > -1:
            print("***************************************************")
            print("**                                             ")
            print("** You are running on a Mac!  This means that  ")
            print("**                                             ")
            print("** Any user using this library must update:    ")
            print("**   DYLD_LIBRARY_PATH                         ")
            print("**                                             ")
            print("** The best way to do this is to put this line:")
            print("**   export DYLD_LIBRARY_PATH=%s" % ("%s/usr/local/lib"
                                                      % installroot))
            print("**                                             ")
            print("** into .bashrc, .profile, or the appropriate")
            print("** shell start-up script!")
            print("***************************************************")
        else:
            print("***************************************************")
            print("**                                             ")
            print("** PyECLib libraries have been installed to:   ")
            print("**   %s/usr/local/lib" % installroot)
            print("**                                             ")
            print("** Any user using this library must update:    ")
            print("**   LD_LIBRARY_PATH                         ")
            print("**                                             ")
            print("** The best way to do this is to put this line:")
            print("**   export LD_LIBRARY_PATH=%s" % ("%s/usr/local/lib"
                                                      % installroot))
            print("**                                             ")
            print("** into .bashrc, .profile, or the appropriate shell")
            print("** start-up script!  Also look at ldconfig(8) man")
            print("** page for a more static LD configuration")
            print("***************************************************")


module = Extension('pyeclib_c',
                   define_macros=[('MAJOR VERSION', '0'),
                                  ('MINOR VERSION', '9')],
                   include_dirs=[default_python_incdir,
                                 '/usr/local/include',
                                 '/usr/include',
                                 'src/c/pyeclib_c',
                                 '%s/include' % c_eclib_dir,
                                 '/usr/local/include'],
                   library_dirs=default_library_paths,
                   runtime_library_dirs=default_library_paths,
                   libraries=['Jerasure', 'Xorcode', 'alg_sig'],
                   # The extra arguments are for debugging
                   # extra_compile_args=['-g', '-O0'],
                   extra_link_args=['-Wl,-rpath,%s' %
                                    l for l in default_library_paths],
                   sources=['src/c/pyeclib_c/pyeclib_c.c'])

setup(name='PyECLib',
      version='0.9.8',
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
      py_modules=['pyeclib.ec_iface', 'pyeclib.core'],
      test_suite='test')
