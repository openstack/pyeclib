from distutils.core import setup, Extension
from distutils.command.install import install as _install
from distutils.command.build import build as _build
import sys
import os

possible_include_dirs = ["/usr/local/include", "/usr/include"]
python_library_name = "python%d.%d" % (sys.version_info.major, sys.version_info.minor)

#
# TODO: Figure out why Pypi is chaning the perms of the files when unpacking...  Is the
# umask set to 022 or something?
#
def _pre_build(dir):
  ret = os.system('(cd c_eclib-0.2 && chmod 755 ./configure && chmod 755 ./install-sh && ./configure && make install)')
  if ret != 0:
    sys.exit(2)

class build(_build):
    def run(self):
        self.execute(_pre_build, (self.build_lib,),
                     msg="Running pre build task(s)")
        _build.run(self)


module = Extension('pyeclib_c',
                   define_macros = [('MAJOR VERSION', '0'),
                                    ('MINOR VERSION', '1')],
                   include_dirs = ['/usr/include/%s' % python_library_name,
                                   'src/c/pyeclib_c',
                                   'c_eclib-0.2/include',
                                   '/usr/local/include'],
                   library_dirs = ['/usr/lib', '/usr/local/lib'],
                   libraries = [python_library_name, 'Jerasure', 'Xorcode'],
                   # The extra arguments are for debugging
                   #extra_compile_args = ['-g', '-O0']
                   extra_link_args=['-Bstatic'],
                   sources = ['src/c/pyeclib_c/pyeclib_c.c'])

setup (name = 'PyECLib',
       version = '0.1.12',
       author = 'Kevin Greenan',
       author_email = 'kmgreen2@gmail.com',
       maintainer = 'Kevin Greenan and Tushar Gohad',
       maintainer_email = 'kmgreen2@gmail.com',
       url = 'https://bitbucket.org/kmgreen2/pyeclib',
       description = 'This library provides a simple Python interface for \
                      implementing erasure codes.  To obtain the best possible performance, the \
                      underlying erasure code algorithms are written in C.',
       platforms = 'Linux',
       license = 'BSD',
       ext_modules = [module],
       packages=['pyeclib'],
       package_dir={'pyeclib': 'src/python/pyeclib'},
       cmdclass={'build': build},
       py_modules = ['pyeclib.ec_iface', 'pyeclib.core'])

