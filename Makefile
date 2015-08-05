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

TOPDIR := $(PWD)

.PHONY: build test

build:
	python setup.py build

install:	build
	python setup.py install

UNITS := test/test_pyeclib_api.py test/test_pyeclib_c.py

test:		build
	$(eval SONAMES := $(shell find $(abs_top_builddir) -name '*.so'))
	$(eval SODIRS := $(dir $(SONAMES)))
	$(eval LD_LIBRARY_PATH := LD_LIBRARY_PATH="$(subst / ,/:,$(SODIRS))")
	$(eval DYLD_LIBRARY_PATH := DYLD_LIBRARY_PATH="$(subst / ,/:,$(SODIRS))")
	$(eval DYLD_FALLBACK_LIBRARY_PATH := DYLD_FALLBACK_LIBRARY_PATH="$(subst / ,/:,$(SODIRS))")
	rm -rf cover .coverage
	@$(LD_LIBRARY_PATH) $(DYLD_LIBRARY_PATH) $(DYLD_FALLBACK_LIBRARY_PATH) \
		nosetests --exe --with-coverage \
		--cover-package pyeclib --cover-erase \
		--cover-html --cover-html-dir=${TOPDIR}/cover \
		$(UNITS)

clean:
	-rm -f pyeclib_c*.so
	-rm -rf build
	python setup.py clean
