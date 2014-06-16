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
import unittest


class TestCoreValgrind(unittest.TestCase):

    def __init__(self, *args):
        self.pyeclib_core_test = "test_pyeclib_c.py"
        self.pyeclib_iface_test = "test_pyeclib_api.py"

        unittest.TestCase.__init__(self, *args)

    def setUp(self):
        # Determine which directory we're in
        dirs = os.getcwd().split('/')
        if dirs[-1] == 'test':
            self.pyeclib_test_dir = "."
        else:
            self.pyeclib_test_dir = "./test"

        # Create the array of tests to run
        self.py_test_dirs = [
            (self.pyeclib_test_dir, self.pyeclib_core_test),
            (self.pyeclib_test_dir, self.pyeclib_iface_test)
        ]

    def tearDown(self):
        pass
    
    @unittest.skipUnless(0 == os.system("which valgrind"), "requires valgrind")
    def test_core_valgrind(self):
        self.assertTrue(True)
        cur_dir = os.getcwd()
        for (dir, test) in self.py_test_dirs:
            os.chdir(dir)
            if os.path.isfile(test):
                ret = os.system(
                    "valgrind --leak-check=full python %s >%s/valgrind.%s.out 2>&1" %
                    (test, cur_dir, test))

                self.assertEqual(0, ret)
                os.system("rm -f *.pyc")
                os.chdir(cur_dir)
            else:
                self.assertTrue(False)

if __name__ == "__main__":
    unittest.main()
