#!/usr/bin/env python

# Copyright (c) 2013-2015, Kevin Greenan (kmgreen2@gmail.com)
# Copyright (c) 2013-2015, Tushar Gohad (tusharsg@gmail.com)
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

from ctypes import *
from ctypes.util import *
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

platform_str = platform.platform()
platform_arch = platform.architecture()
default_python_incdir = get_python_inc()
default_python_libdir = get_python_lib()
default_library_paths = [default_python_libdir]


# utility routines
def _find_library(name):
    target_lib = None
    if os.name == 'posix' and sys.platform.startswith('linux'):
        from ctypes.util import _findLib_gcc
        target_lib = _findLib_gcc(name)
    else:
        target_lib = find_library(name)
    if target_lib:
        target_lib = os.path.abspath(target_lib)
        if os.path.islink(target_lib):
            p = os.readlink(target_lib)
            if os.path.isabs(p):
                target_lib = p
            else:
                target_lib = os.path.join(os.path.dirname(target_lib), p)
    # return absolute path to the library if found
    return target_lib


def _build_default_lib_search_path():
    default_library_paths.append('/usr/local/lib')
    arch64 = platform_arch[0].startswith('64')
    for prefix in ('/', '/usr', '/usr/local', _exec_prefix):
        libdir = os.path.join(prefix, 'lib')
        libdir64 = os.path.join(prefix, 'lib64')
        if arch64 and os.path.exists(libdir64):
            default_library_paths.append(libdir64)
        else:
            default_library_paths.append(libdir)
    return default_library_paths


def _read_file_as_str(name):
    with open(name, "rt") as f:
        s = f.readline().strip()
    return s


def _liberasurecode_install_error(library, library_url):
    print("***                                                ")
    print("*** Please install " + library + " manually.       ")
    print("*** project url: %s" % library_url)


install_libec = True


class build(_build):

    boolean_options = _build.boolean_options + ['install-liberasurecode']
    user_options = _build.user_options + [
        ('install-liberasurecode=', None,
         'Install liberasurecode dependency (yes/no) [default: yes].')
    ]

    def initialize_options(self):
        self.install_liberasurecode = True
        _build.initialize_options(self)

    def check_liberasure(self):
        library_basename = "liberasurecode"
        library_version = "1.1.0"
        # try using an integrated copy of the library
        library = library_basename + "-" + library_version
        library_url = "https://bitbucket.org/tsg-/liberasurecode.git"

        if platform_str.find("Darwin") > -1:
            liberasure_file = \
                library_basename + "." + library_version + ".dylib"
        else:
            liberasure_file = \
                library_basename + ".so." + library_version

        notfound = True
        found_path = _find_library("erasurecode")

        if found_path:
            if found_path.endswith(library_version) or \
                    found_path.find(library_version + ".") > -1:
                # call 1.1.0 the only compatible version for now
                notfound = False

        if found_path and notfound:
            # look harder
            _build_default_lib_search_path()
            for dir in (default_library_paths):
                liberasure_file_path = dir + os.sep + liberasure_file
                if (os.path.isfile(liberasure_file_path)):
                    notfound = False
                    break

        if not notfound:
            return

        install_status = 0

        print("**************************************************************")
        print("***                                                           ")
        print("*** Can not locate %s" % (liberasure_file))

        if install_libec:
            print("**                                                 ")
            print("** PyECLib requires liberasurecode.  Trying to     ")
            print("** install using bundled tarball.                  ")
            print("**                                                 ")
            print("** If you have liberasurecode already installed,   ")
            print("** you may need to run 'sudo ldconfig' to update   ")
            print("** the loader cache.                               ")

            srcpath = "src/c/"
            locallibsrcdir = (srcpath + library)
            os.system("rm -r %s" % locallibsrcdir)
            retval = os.system("tar -xzf %s/%s.tar.gz -C %s" %
                               (srcpath, library, srcpath))
            if (os.path.isdir(locallibsrcdir)):
                curdir = os.getcwd()
                os.chdir(locallibsrcdir)
                configure_cmd = ("./configure --prefix=/usr/local")
                print(configure_cmd)
                install_status = os.system(configure_cmd)
                if install_status != 0:
                    _liberasurecode_install_error(library, library_url)
                    os.chdir(curdir)
                make_cmd = ("make && make install")
                install_status = os.system(make_cmd)
                if install_status != 0:
                    _liberasurecode_install_error(library, library_url)
                    os.chdir(curdir)
                os.chdir(curdir)
        else:
            _liberasurecode_install_error(library, library_url)
            install_status = -1

        print("***                                                           ")
        print("**************************************************************")

        if install_status != 0:
            sys.exit(install_status)

    def run(self):
        global install_libec

        if not self.install_liberasurecode:
            install_libec = False

        self.check_liberasure()
        _build.run(self)


class clean(_clean):

    def run(self):
        _clean.run(self)


class install(_install):

    boolean_options = _install.boolean_options + ['install-liberasurecode']
    user_options = _install.user_options + [
        ('install-liberasurecode=', None,
         'Install liberasurecode dependency (yes/no) [default: yes].')
    ]

    def initialize_options(self):
        self.install_liberasurecode = True
        _install.initialize_options(self)

    def run(self):
        global install_libec

        if not self.install_liberasurecode:
            install_libec = False

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

        installroot = install_lib.install_dir
        _install.run(self)

        # Another Mac-ism...  If the libraries are installed
        # in a strange place, DYLD_LIRBARY_PATH needs to be
        # updated.
        if platform_str.find("Darwin") > -1:
            ldpath_str = "DYLD_LIBRARY_PATH"
        else:
            ldpath_str = "LD_LIBRARY_PATH"
        print("***************************************************")
        print("**                                                 ")
        print("** PyECLib libraries have been installed to:       ")
        print("**   %s" % installroot)
        print("**                                                 ")
        print("** Any user using this library must update:        ")
        print("**   %s" % ldpath_str)
        print("**                                                 ")
        print("** Run 'ldconfig' or place this line:              ")
        print("**   export %s=%s" % (ldpath_str, "%s"
                                     % installroot))
        print("**                                                 ")
        print("** into .bashrc, .profile, or the appropriate shell")
        print("** start-up script!  Also look at ldconfig(8) man  ")
        print("** page for a more static LD configuration         ")
        print("**                                                 ")
        print("***************************************************")


module = Extension('pyeclib_c',
                   define_macros=[('MAJOR VERSION', '1'),
                                  ('MINOR VERSION', '0')],
                   include_dirs=[default_python_incdir,
                                 '/usr/local/include/liberasurecode',
                                 '/usr/local/include/jerasure',
                                 '/usr/include/liberasurecode',
                                 '/usr/include',
                                 'src/c/pyeclib_c',
                                 '/usr/local/include'],
                   runtime_library_dirs=default_library_paths,
                   libraries=['erasurecode'],
                   # The extra arguments are for debugging
                   # extra_compile_args=['-g', '-O0'],
                   sources=['src/c/pyeclib_c/pyeclib_c.c'])

setup(name='PyECLib',
      version='1.1.1',
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
      package_dir={'pyeclib': 'pyeclib'},
      cmdclass={'build': build, 'install': install, 'clean': clean},
      py_modules=['pyeclib.ec_iface', 'pyeclib.core'],
      test_suite='test')
