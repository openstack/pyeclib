import sys
import os

base_c_dir="../src/c"
xor_code_dir="%s/xor_codes" % (base_c_dir)
alg_sig_dir="%s/alg_sig" % (base_c_dir)
pyeclib_core_dir="%s/pyeclib" % (base_c_dir)
xor_code_test = "./test_xor_hd_code"
alg_sig_test = "./alg_sig_test"
pyeclib_core_test = "pyeclib_test.py"
pyeclib_swift_test = "ec_test.py"
base_python_dir = "../src/python"
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

def pyeclib_core_valgrind():
  cur_dir = os.getcwd()
  for (dir, test) in py_test_dirs:
    os.chdir(dir)
    ret = os.system("valgrind --leak-check=full python %s >%s/valgrind.%s.out 2>&1" % (test, cur_dir, test))
    if ret != 0:
      print "Building %s failed!!!" % (dir)
      sys.exit(1) 
    os.system("rm -f *.pyc")
    os.chdir(cur_dir)

  
test_c_stuff()
pyeclib_core_test()
#pyeclib_core_valgrind()

