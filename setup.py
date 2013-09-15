from distutils.core import setup, Extension

module = Extension('pyeclib',
                   define_macros = [('MAJOR VERSION', '1'),
                                    ('MINOR VERSION', '0')],
                   include_dirs = ['/usr/include/python2.7',
                                   'src/c/pyeclib',
                                   '/usr/local/include/jerasure'],
                   library_dirs = ['/usr/lib', '/usr/local/lib'],
                   libraries = ['python2.7', 'Jerasure'],
                   # The extra arguments are for debugging
                   extra_compile_args = ['-g', '-O0'],
                   extra_link_args=['-static'],
                   sources = ['src/c/pyeclib/pyeclib.c'])

setup (name = 'PyECLib',
       version = '0.1',
       ext_modules = [module])
