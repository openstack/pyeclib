CFLAGS=-O3 -msse3
GCC=gcc
INCLUDE_DIRS=-I./ -I../

all: test_xor_hd_code
  
test_xor_hd_code: xor_code.o xor_hd_code.o test_xor_hd_code.o common.o
	${GCC} ${CFLAGS} ${INCLUDE_DIRS} xor_code.o xor_hd_code.o test_xor_hd_code.o common.o -o test_xor_hd_code

xor_code.o: xor_code.c 
	${GCC} ${CFLAGS} ${INCLUDE_DIRS} -c xor_code.c

xor_hd_code.o: xor_hd_code.c 
	${GCC} ${CFLAGS} ${INCLUDE_DIRS} -c xor_hd_code.c

test_xor_hd_code.o: test_xor_hd_code.c
	${GCC} ${CFLAGS} ${INCLUDE_DIRS} -c test_xor_hd_code.c

common.o: ../common.c
	${GCC} ${CFLAGS} ${INCLUDE_DIRS} -c ../common.c

clean:
	rm -rf test_xor_hd_code *.o
