# XOR-based erasure codes (including HD Combination Codes)

INCLUDES=-I./ -I../include
AM_CFLAGS = -O3 $(SIMD_FLAGS) -fPIC $(INCLUDES)

lib_LTLIBRARIES = libXorcode.la
libXorcode_la_SOURCES = xor_code.c xor_hd_code.c

bin_PROGRAMS = test_xor_hd_code
test_xor_hd_code_SOURCES = test_xor_hd_code.c
test_xor_hd_code_LDFLAGS = -lXorcode

