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
import re
import subprocess
import sys

import ctypes
from ctypes.util import find_library
from distutils.command.build import build as _build
from distutils.command.clean import clean as _clean
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
default_python_incdir = get_python_inc()


# this is to be used only for library existence/version checks,
# not for rpath handling
def _find_library(name):
    target_lib = find_library(name)
    if platform_str.find("Darwin") > -1:
        # If we didn't find it, try extending our search a bit
        if not target_lib:
            if 'DYLD_LIBRARY_PATH' in os.environ:
                os.environ['DYLD_LIBRARY_PATH'] += ':%s/lib' % sys.prefix
            else:
                os.environ['DYLD_LIBRARY_PATH'] = '%s/lib' % sys.prefix
            target_lib = find_library(name)

        # If we *still* don't find it, bail
        if not target_lib:
            return target_lib

        target_lib = os.path.abspath(target_lib)
        if os.path.islink(target_lib):
            p = os.readlink(target_lib)
            if os.path.isabs(p):
                target_lib = p
            else:
                target_lib = os.path.join(os.path.dirname(target_lib), p)
    elif not target_lib:
        # See https://bugs.python.org/issue9998
        expr = r'[^\(\)\s]*lib%s\.[^\(\)\s]*' % re.escape(name)
        cmd = ['ld', '-t']
        libpath = os.environ.get('LD_LIBRARY_PATH')
        if libpath:
            for d in libpath.split(':'):
                cmd.extend(['-L', d])
        cmd.extend(['-o', os.devnull, '-l%s' % name])
        try:
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE,
                                 universal_newlines=True)
            out, _ = p.communicate()
            if hasattr(os, 'fsdecode'):
                out = os.fsdecode(out)
            res = re.search(expr, out)
            if res:
                target_lib = res.group(0)
        except Exception:
            pass  # result will be None
    # return absolute path to the library if found
    return target_lib


class build(_build):

    def check_liberasure(self):
        library_basename = "liberasurecode"
        library_version = "1"
        library_url = "https://github.com/openstack/liberasurecode"

        found_path = _find_library("erasurecode")
        if found_path:
            libec = ctypes.CDLL(found_path)
            try:
                packed_version = libec.liberasurecode_get_version()
            except AttributeError:
                # If we don't have the version getter, we're probably
                # pre-1.4.0; fall back to some heuristics based on name
                version_parts = [x for x in found_path.split('.')
                                 if x.isdigit()]
                if not version_parts:
                    # A bare .so? Well, we haven't released a 2.x yet... but
                    # if we ever do, hopefully it'd still provide a
                    # liberasurecode_get_version()
                    return
                if version_parts[0] == library_version:
                    return
                # else, seems to be an unknown version -- assume it won't work
            else:
                version = (
                    packed_version >> 16,
                    (packed_version >> 8) & 0xff,
                    packed_version & 0xff)
                if (1, 0, 0) <= version < (2, 0, 0):
                    return

        if platform_str.find("Darwin") > -1:
            liberasure_file = \
                library_basename + "." + library_version + ".dylib"
        else:
            liberasure_file = \
                library_basename + ".so." + library_version

        print("**************************************************************")
        print("***                                                           ")
        print("*** Can not locate %s" % (liberasure_file))
        print("***                                                ")
        print("*** Install - ")
        print("***   Manual: %s" % library_url)
        print("***   Fedora/Red Hat variants: liberasurecode-devel")
        print("***   Debian/Ubuntu variants: liberasurecode-dev")
        print("***                                                           ")
        print("**************************************************************")

        sys.exit(-1)

    def run(self):
        self.check_liberasure()
        _build.run(self)


class clean(_clean):

    def run(self):
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
                                  ('MINOR VERSION', '6')],
                   include_dirs=[default_python_incdir,
                                 'src/c/pyeclib_c',
                                 '/usr/include',
                                 '/usr/include/liberasurecode',
                                 '%s/include/liberasurecode' % sys.prefix,
                                 '%s/include' % sys.prefix],
                   libraries=['erasurecode'],
                   library_dirs=['%s/lib' % sys.prefix],
                   runtime_library_dirs=['%s/lib' % sys.prefix],
                   # The extra arguments are for debugging
                   # extra_compile_args=['-g', '-O0'],
                   sources=['src/c/pyeclib_c/pyeclib_c.c'])

setup(name='pyeclib',
      version='1.6.0',
      author='Kevin Greenan',
      author_email='kmgreen2@gmail.com',
      maintainer='Kevin Greenan and Tushar Gohad',
      maintainer_email='kmgreen2@gmail.com, tusharsg@gmail.com',
      url='http://git.openstack.org/cgit/openstack/pyeclib/',
      bugtrack_url='https://bugs.launchpad.net/pyeclib',
      description=('This library provides a simple Python interface for '
                   'implementing erasure codes.  To obtain the best possible '
                   'performance, the underlying erasure code algorithms are '
                   'written in C.'),
      long_description=open('README.rst', 'r').read(),
      platforms='Linux',
      license='BSD',
      ext_modules=[module],
      packages=['pyeclib'],
      package_dir={'pyeclib': 'pyeclib'},
      cmdclass={'build': build, 'install': install, 'clean': clean},
      py_modules=['pyeclib.ec_iface', 'pyeclib.core'],
      command_options={
          'build_sphinx': {
              'build_dir': ('setup.py', 'doc/build')}},
      test_suite='test')
