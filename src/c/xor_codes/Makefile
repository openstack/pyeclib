CFLAGS=-O3 -msse3
GCC=gcc
INCLUDE_DIRS=.

all: test_xor_hd_code
  
test_xor_hd_code: xor_code.o xor_hd_code.o test_xor_hd_code.o
	${GCC} ${CFLAGS} -I${INCLUDE_DIRS} xor_code.o xor_hd_code.o test_xor_hd_code.o -o test_xor_hd_code

xor_code.o: xor_code.c
	${GCC} ${CFLAGS} -I${INCLUDE_DIRS} -c xor_code.c

xor_hd_code.o: xor_hd_code.c
	${GCC} ${CFLAGS} -I${INCLUDE_DIRS} -c xor_hd_code.c

test_xor_hd_code.o: test_xor_hd_code.c
	${GCC} ${CFLAGS} -I${INCLUDE_DIRS} -c test_xor_hd_code.c

clean:
	rm -rf test_xor_hd_code *.o
