from distutils.core import setup, Extension
import sys
import os

module = Extension('pyeclib_c',
                   define_macros = [('MAJOR VERSION', '1'),
                                    ('MINOR VERSION', '0')],
                   include_dirs = ['/usr/include/python2.7',
                                   'src/c/pyeclib_c',
                                   'src/c/include',
                                   'src/c/xor_codes',
                                   '/usr/local/include/jerasure'],
                   library_dirs = ['/usr/lib', '/usr/local/lib'],
                   libraries = ['python2.7', 'Jerasure'],
                   # The extra arguments are for debugging
                   #extra_compile_args = ['-g', '-O0', '-msse3'],
                   extra_compile_args = ['-msse3', '-fPIC'],
                   extra_link_args=['-Bstatic'],
                   sources = ['src/c/pyeclib_c/pyeclib_c.c', 'c_eclib/xor_codes/xor_code.c', 'c_eclib/xor_codes/xor_hd_code.c'])

setup (name = 'PyECLib',
       version = '0.1',
       ext_modules = [module],
       packages=['pyeclib'],
       package_dir={'pyeclib': 'src/python/pyeclib'},
       py_modules = ['pyeclib.ec_iface', 'pyeclib.core'])

