# Copyright (c) 2025, NVIDIA
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

import io
import platform
import re
import unittest
from unittest import mock

from pyeclib.cli.__main__ import main
from pyeclib import ec_iface


class TestVersion(unittest.TestCase):
    def _test_version(self, args):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                self.assertRaises(SystemExit) as caught:
            main(args)
        self.assertTrue(stdout.getvalue().endswith("\n"))
        line_parts = [
            line.split(" ")
            for line in stdout.getvalue()[:-1].split("\n")
        ]
        self.assertEqual([prog for prog, vers in line_parts], [
            'pyeclib',
            'liberasurecode',
            platform.python_implementation(),
        ])
        re_version = re.compile(r"\d+\.\d+\.\d+")
        self.assertEqual(
            [bool(re_version.match(vers)) for prog, vers in line_parts],
            [True] * 3)
        self.assertEqual(caught.exception.code, None)

    def test_subcommand(self):
        self._test_version(["version"])

    def test_opt(self):
        self._test_version(["-V"])


class TestList(unittest.TestCase):
    def test_list(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                self.assertRaises(SystemExit) as caught:
            main(["list"])
        self.assertTrue(stdout.getvalue().endswith("\n"))
        line_parts = [
            line.split()
            for line in stdout.getvalue()[:-1].split("\n")
        ]
        self.assertEqual(
            [backend for backend, status in line_parts],
            sorted(ec_iface.ALL_EC_TYPES))
        self.assertEqual(
            {status for backend, status in line_parts},
            {"available", "missing"})
        self.assertEqual(caught.exception.code, 0)

    def test_list_one(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                self.assertRaises(SystemExit) as caught:
            main(["list", "liberasurecode_rs_vand"])
        self.assertTrue(stdout.getvalue().endswith("\n"))
        line_parts = [
            line.split()
            for line in stdout.getvalue()[:-1].split("\n")
        ]
        self.assertEqual(line_parts, [["liberasurecode_rs_vand", "available"]])
        self.assertEqual(caught.exception.code, 0)

    def test_list_one_unknown(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                self.assertRaises(SystemExit) as caught:
            main(["list", "missing-backend"])
        self.assertTrue(stdout.getvalue().endswith("\n"))
        line_parts = [
            line.split()
            for line in stdout.getvalue()[:-1].split("\n")
        ]
        self.assertEqual(line_parts, [["missing-backend", "unknown"]])
        self.assertEqual(caught.exception.code, 1)

    def test_list_multiple_mixed_status(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                self.assertRaises(SystemExit) as caught:
            main(["list", "missing-backend", "liberasurecode_rs_vand"])
        self.assertTrue(stdout.getvalue().endswith("\n"))
        line_parts = [
            line.split()
            for line in stdout.getvalue()[:-1].split("\n")
        ]
        self.assertEqual(line_parts, [
            ["liberasurecode_rs_vand", "available"],
            ["missing-backend", "unknown"],
        ])
        self.assertEqual(caught.exception.code, 0)

    def test_list_abbreviation(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                self.assertRaises(SystemExit) as caught:
            main(["list", "isal"])
        self.assertTrue(stdout.getvalue().endswith("\n"))
        line_parts = [
            line.split()
            for line in stdout.getvalue()[:-1].split("\n")
        ]
        self.assertEqual(
            [backend for backend, status in line_parts],
            sorted(backend for backend in ec_iface.ALL_EC_TYPES
                   if backend.startswith("isa_l_")))
        found_status = {status for backend, status in line_parts}
        self.assertEqual(len(found_status), 1, found_status)
        found_status = list(found_status)[0]
        self.assertIn(found_status, {"available", "missing"})
        self.assertEqual(caught.exception.code,
                         0 if found_status == "available" else 1)

    def test_list_available(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                self.assertRaises(SystemExit) as caught:
            main(["list", "--available"])
        self.assertTrue(stdout.getvalue().endswith("\n"))
        self.assertEqual(
            stdout.getvalue()[:-1].split("\n"),
            sorted(ec_iface.VALID_EC_TYPES))
        self.assertEqual(caught.exception.code, 0)

    def test_list_available_abbreviation(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                self.assertRaises(SystemExit) as caught:
            main(["list", "--available", "flatxor"])
        self.assertTrue(stdout.getvalue().endswith("\n"))
        self.assertEqual(stdout.getvalue()[:-1].split("\n"), [
            "flat_xor_hd_3",
            "flat_xor_hd_4",
        ])
        self.assertEqual(caught.exception.code, 0)


class TestCheck(unittest.TestCase):
    def test_check_backend_required(self):
        with mock.patch("sys.stderr", new=io.StringIO()) as stderr, \
                self.assertRaises(SystemExit) as caught:
            main(["check"])
        self.assertIn("the following arguments are required: ec_type",
                      stderr.getvalue())
        self.assertEqual(caught.exception.code, 2)

    def test_check_available(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                mock.patch("sys.stderr", new=io.StringIO()) as stderr, \
                self.assertRaises(SystemExit) as caught:
            main(["check", "liberasurecode_rs_vand"])
        self.assertEqual("liberasurecode_rs_vand is available\n",
                         stdout.getvalue())
        self.assertEqual("", stderr.getvalue())
        self.assertEqual(caught.exception.code, 0)

    def test_check_missing(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                mock.patch("sys.stderr", new=io.StringIO()) as stderr, \
                mock.patch("pyeclib.ec_iface.VALID_EC_TYPES", []), \
                self.assertRaises(SystemExit) as caught:
            main(["check", "liberasurecode_rs_vand"])
        self.assertEqual("liberasurecode_rs_vand is missing\n",
                         stdout.getvalue())
        self.assertEqual("", stderr.getvalue())
        self.assertEqual(caught.exception.code, 1)

    def test_check_unknown(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                mock.patch("sys.stderr", new=io.StringIO()) as stderr, \
                mock.patch("pyeclib.ec_iface.VALID_EC_TYPES", []), \
                self.assertRaises(SystemExit) as caught:
            main(["check", "unknown-backend"])
        self.assertEqual("unknown-backend is unknown\n",
                         stdout.getvalue())
        self.assertEqual("", stderr.getvalue())
        self.assertEqual(caught.exception.code, 2)

    def test_check_quiet_available(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                mock.patch("sys.stderr", new=io.StringIO()) as stderr, \
                self.assertRaises(SystemExit) as caught:
            main(["check", "-q", "liberasurecode_rs_vand"])
        self.assertEqual("", stdout.getvalue())
        self.assertEqual("", stderr.getvalue())
        self.assertEqual(caught.exception.code, 0)

    def test_check_quiet_missing(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                mock.patch("sys.stderr", new=io.StringIO()) as stderr, \
                mock.patch("pyeclib.ec_iface.VALID_EC_TYPES", []), \
                self.assertRaises(SystemExit) as caught:
            main(["check", "--quiet", "liberasurecode_rs_vand"])
        self.assertEqual("", stdout.getvalue())
        self.assertEqual("", stderr.getvalue())
        self.assertEqual(caught.exception.code, 1)

    def test_check_quiet_unknown(self):
        with mock.patch("sys.stdout", new=io.StringIO()) as stdout, \
                mock.patch("sys.stderr", new=io.StringIO()) as stderr, \
                mock.patch("pyeclib.ec_iface.VALID_EC_TYPES", []), \
                self.assertRaises(SystemExit) as caught:
            main(["check", "-q", "unknown-backend"])
        self.assertEqual("", stdout.getvalue())
        self.assertEqual("", stderr.getvalue())
        self.assertEqual(caught.exception.code, 2)
