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

import sys
import os

import unittest


base_c_dir = "../src/c"
xor_code_dir = "%s/xor_codes" % (base_c_dir)
alg_sig_dir = "%s/alg_sig" % (base_c_dir)
pyeclib_core_dir = "%s/pyeclib" % (base_c_dir)
xor_code_test = "/usr/local/bin/test_xor_hd_code"
alg_sig_test = "/usr/local/bin/alg_sig_test"
pyeclib_core_test = "pyeclib_test.py"
pyeclib_iface_test = "ec_test.py"
base_python_dir = "../src/python"
pyeclib_core_test_dir = "%s/test" % (base_python_dir)
pyeclib_iface_test_dir = "%s/pyeclib" % (base_python_dir)
pyeclib_core_test = "pyeclib_test.py"
pyeclib_iface_test = "ec_test.py"
c_build_dirs = [(xor_code_dir, xor_code_test), (alg_sig_dir, alg_sig_test)]
py_test_dirs = [
    (pyeclib_core_test_dir,
     pyeclib_core_test),
    (pyeclib_iface_test_dir,
     pyeclib_iface_test)]
     

class TestCoreC(unittest.TestCase):
    """
    TODO: ensure that all these tests run without strange directory acrobatics
    """
    def setUp(self):
        pass

    def tearDown(self):
        pass

    @unittest.skip("Test refactoring")
    def test_c_stuff(self):
        self.assertTrue(True)
        cur_dir = os.getcwd()
        for (dir, test) in c_build_dirs:
            self.assertEqual(0, os.system(test))


class TestCoreTest(unittest.TestCase):
    """
    TODO: migrate src/python/test/pyeclib_test.py to /test
    """
    def setUp(self):
        pass

    def tearDown(self):
        pass

    @unittest.skip("Test refactoring")
    def test_core(self):
        self.assertTrue(True)
        cur_dir = os.getcwd()
        for (dir, test) in py_test_dirs:
            print("Dir: %s, test: %s" % (dir, test))
            """
            os.chdir(dir)
            ret = os.system("python %s" % test)
            if ret != 0:
                print("Building %s failed!!!" % (dir))
                sys.exit(1)
            os.system("rm -f *.pyc")
            os.chdir(cur_dir)
            """


class TestCoreValgrind(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass
    
    @unittest.skipUnless(0 == os.system("which valgrind"), "requires valgrind")
    def test_core_valgrind(self):
        self.assertTrue(True)
        """
        cur_dir = os.getcwd()
        for (dir, test) in py_test_dirs:
            os.chdir(dir)
            ret = os.system(
                "valgrind --leak-check=full python %s >%s/valgrind.%s.out 2>&1" %
                (test, cur_dir, test))
            if ret != 0:
                print("Building %s failed!!!" % (dir))
                sys.exit(1)
            os.system("rm -f *.pyc")
            os.chdir(cur_dir)
        """

if __name__ == "__main__":
    unittest.main()
