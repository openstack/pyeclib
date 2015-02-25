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
import sys

run_under_valgrind = False
test_cmd_prefix = ""
log_filename_prefix = ""


class CoreTests():

    def __init__(self, *args):
        self.pyeclib_core_test = "test_pyeclib_c.py"
        self.pyeclib_iface_test = "test_pyeclib_api.py"

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

    def invoke_core_tests(self):
        cur_dir = os.getcwd()
        print("\n")
        for (dir, test) in self.py_test_dirs:
            sys.stdout.write("Running test %s ... " % test)
            sys.stdout.flush()
            os.chdir(dir)
            if os.path.isfile(test):
                pythonpath = "PYTHONPATH=%s:%s" % \
                    (cur_dir, os.path.dirname(cur_dir))
                ret = os.system(
                    "%s %s python %s >%s/%s.%s.out 2>&1" %
                    (pythonpath, test_cmd_prefix, test, cur_dir,
                     log_filename_prefix, test))

                assert(0 == ret)
                os.system("rm -f *.pyc")
                os.chdir(cur_dir)
                print('ok')
            else:
                print('failed')


# Invoke this script as "python test_core_valgrind.py"
# for the "valgrind" variant
# (test_core_valgrind.py is a symlink to test_core.py)
if __name__ == "__main__":
    if '_valgrind' in sys.argv[0]:
        if (0 != os.system("which valgrind")):
            print("You don't appear to have 'valgrind' installed")
            sys.exit(-1)
        run_under_valgrind = True
        test_cmd_prefix = "valgrind --leak-check=full "
        log_filename_prefix = "valgrind"
    coretests = CoreTests()
    coretests.setUp()
    coretests.invoke_core_tests()
    coretests.tearDown()
