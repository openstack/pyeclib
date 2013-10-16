# XOR-based erasure codes (including HD Combination Codes)

INCLUDES=-I./ -I../include -I../jerasure
AM_CFLAGS = -O3 $(SIMD_FLAGS) -fPIC $(INCLUDES) -L../jerasure -L../gf-complete

lib_LTLIBRARIES = libalg_sig.la
libalg_sig_la_SOURCES = alg_sig.c
libalg_sig_la_LDFLAGS = -version-info 0:1:0
libalg_sig_la_ADDLIB = -lgf_complete

bin_PROGRAMS = alg_sig_test
alg_sig_test_SOURCES = alg_sig_test.c
alg_sig_test_LDFLAGS = -lalg_sig -lJerasure -lgf_complete

