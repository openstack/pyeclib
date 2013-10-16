# XOR-based erasure codes (including HD Combination Codes)

INCLUDES=-I./ -I../include
AM_CFLAGS = -O3 $(SIMD_FLAGS) -fPIC $(INCLUDES)

lib_LTLIBRARIES = libalg_sig.la
libalg_sig_la_SOURCES = alg_sig.c

bin_PROGRAMS = alg_sig_test
alg_sig_test_SOURCES = alg_sig_test.c
alg_sig_test_LDFLAGS = -lalg_sig

