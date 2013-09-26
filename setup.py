from distutils.core import setup, Extension
import sys
import os

base_c_dir="src/c"
xor_code_dir="%s/xor_codes" % (base_c_dir)
alg_sig_dir="%s/alg_sig" % (base_c_dir)
pyeclib_core_dir="%s/pyeclib" % (base_c_dir)
xor_code_test = "./test_xor_hd_code"
alg_sig_test = "./alg_sig_test"
pyeclib_core_test = "pyeclib_test.py"
pyeclib_swift_test = "ec_test.py"
base_python_dir = "src/python"
pyeclib_core_test_dir = "%s/test" % (base_python_dir)
pyeclib_swift_test_dir = "%s/swift" % (base_python_dir)
pyeclib_core_test = "pyeclib_test.py"
pyeclib_swift_test = "ec_test.py"
c_build_dirs=[(xor_code_dir, xor_code_test), (alg_sig_dir, alg_sig_test)]
py_test_dirs=[(pyeclib_core_test_dir, pyeclib_core_test), (pyeclib_swift_test_dir, pyeclib_swift_test)]

def test_c_stuff():
  cur_dir = os.getcwd()
  for (dir, test) in c_build_dirs:
    os.chdir(dir)
    ret = os.system("make")
    if ret != 0:
      print "Building %s failed!!!" % (dir)
      sys.exit(1) 
    os.system(test)
    ret = os.system("make clean")
    os.chdir(cur_dir)

def pyeclib_core_test():
  cur_dir = os.getcwd()
  for (dir, test) in py_test_dirs:
    os.chdir(dir)
    ret = os.system("python %s" % test)
    if ret != 0:
      print "Building %s failed!!!" % (dir)
      sys.exit(1) 
    os.system("rm -f *.pyc")
    os.chdir(cur_dir)
  
run_tests = False
if "run_tests" in sys.argv:
  index = sys.argv.index("run_tests")
  sys.argv.pop(index)
  run_tests = True

    
module = Extension('pyeclib',
                   define_macros = [('MAJOR VERSION', '1'),
                                    ('MINOR VERSION', '0')],
                   include_dirs = ['/usr/include/python2.7',
                                   'src/c/pyeclib',
                                   'src/c',
                                   'src/c/xor_codes',
                                   '/usr/local/include/jerasure'],
                   library_dirs = ['/usr/lib', '/usr/local/lib'],
                   libraries = ['python2.7', 'Jerasure'],
                   # The extra arguments are for debugging
                   #extra_compile_args = ['-g', '-O0', '-msse3'],
                   extra_compile_args = ['-msse3'],
                   extra_link_args=['-static'],
                   sources = ['src/c/pyeclib/pyeclib.c', 'src/c/xor_codes/xor_code.c', 'src/c/xor_codes/xor_hd_code.c', 'src/c/common.c'])

setup (name = 'PyECLib',
       version = '0.1',
       ext_modules = [module])

if run_tests is True:
  test_c_stuff()
  pyeclib_core_test()

