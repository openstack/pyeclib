CFLAGS=-O3 -msse3
GCC=gcc
INCLUDE_DIRS=-I./ -I../include

all: test_xor_hd_code ../lib/libXorcode.so

../lib/libXorcode.so: ../lib ../lib/libXorcode.so.0
	ln -sf libXorcode.so.0 ../lib/libXorcode.so

../lib:
	mkdir ../lib

../lib/libXorcode.so.0: ../lib xor_code.o xor_hd_code.o
	$(CC) -shared -Wl -o ../lib/libXorcode.so.0 xor_code.o xor_hd_code.o
  
test_xor_hd_code: xor_code.o xor_hd_code.o test_xor_hd_code.o
	${GCC} ${CFLAGS} ${INCLUDE_DIRS} xor_code.o xor_hd_code.o test_xor_hd_code.o -o test_xor_hd_code

xor_code.o: xor_code.c 
	${GCC} ${CFLAGS} ${INCLUDE_DIRS} -c xor_code.c

xor_hd_code.o: xor_hd_code.c 
	${GCC} ${CFLAGS} ${INCLUDE_DIRS} -c xor_hd_code.c

test_xor_hd_code.o: test_xor_hd_code.c
	${GCC} ${CFLAGS} ${INCLUDE_DIRS} -c test_xor_hd_code.c

clean:
	rm -rf test_xor_hd_code *.o ../lib/libXorcode.so ../lib/libXorcode.so.0
