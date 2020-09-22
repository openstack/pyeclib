# Copyright (c) 2013, 2014, Kevin Greenan (kmgreen2@gmail.com)
# Copyright (c) 2014, Tushar Gohad (tusharsg@gmail.com)
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
import traceback


def positive_int_value(param):
    # Returns value as a positive int or raises ValueError otherwise
    try:
        value = int(param)
        assert value > 0
    except (TypeError, ValueError, AssertionError):
        # Handle: TypeError for 'None', ValueError for non-int strings
        # and AssertionError for values <= 0
        raise ValueError('Must be an integer > 0, not "%s".' % param)
    return value


def import_class(import_str):
    """
    Returns a class from a string that specifies a module and/or class

    :param import_str: import path, e.g. 'httplib.HTTPConnection'
    :returns imported object
    :raises: ImportedError if the class does not exist or the path is invalid
    """
    (mod_str, separator, class_str) = import_str.rpartition('.')
    try:
        __import__(mod_str)
        return getattr(sys.modules[mod_str], class_str)
    except (ValueError, AttributeError):
        raise ImportError('Class %s cannot be found (%s)' %
                          (class_str,
                           traceback.format_exception(*sys.exc_info())))


def create_instance(import_str, *args, **kwargs):
    """
    Returns instance of class which imported by import path.

    :param import_str: import path of class
    :param *args: indexed arguments for new instance
    :param **kwargs: keyword arguments for new instance
    :returns: instance of imported class which instantiated with
    arguments *args and **kwargs
    """
    try:
        object_class = import_class(import_str)
    except Exception:
        raise
    instance = object_class(*args, **kwargs)

    return instance
