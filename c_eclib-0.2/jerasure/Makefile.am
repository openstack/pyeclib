# Jerasure AM file
# Only creates library and distributes header files
# Use orig.makfile if you want to build everything

INCLUDES=-I./ -I../gf-complete 
AM_CFLAGS = -O3 $(SIMD_FLAGS) -fPIC $(INCLUDES) -L../gf-complete

lib_LTLIBRARIES = libJerasure.la
libJerasure_la_SOURCES = galois.c jerasure.c reed_sol.c cauchy.c liberation.c
libJerasure_la_LDFLAGS = -version-info 2:0:0
libJerasure_la_LIBADD = -lgf_complete
include_HEADERS = jerasure.h galois.h reed_sol.h cauchy.h liberation.h
