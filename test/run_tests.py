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

base_c_dir = "../src/c"
pyeclib_core_dir = "%s/pyeclib" % (base_c_dir)
pyeclib_core_test = "pyeclib_test.py"
pyeclib_iface_test = "ec_test.py"
base_python_dir = "../src/python"
pyeclib_core_test_dir = "%s/test" % (base_python_dir)
pyeclib_iface_test_dir = "%s/pyeclib" % (base_python_dir)
pyeclib_core_test = "pyeclib_test.py"
pyeclib_iface_test = "ec_test.py"
py_test_dirs = [
    (pyeclib_core_test_dir,
     pyeclib_core_test),
    (pyeclib_iface_test_dir,
     pyeclib_iface_test)]


def pyeclib_core_test():
    cur_dir = os.getcwd()
    for (dir, test) in py_test_dirs:
        os.chdir(dir)
        ret = os.system("python %s" % test)
        if ret != 0:
            print("Building %s failed!!!" % (dir))
            sys.exit(1)
        os.system("rm -f *.pyc")
        os.chdir(cur_dir)


def pyeclib_core_valgrind():
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


pyeclib_core_test()
# pyeclib_core_valgrind()
