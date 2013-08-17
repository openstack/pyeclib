SOURCES=xor_code.c xor_code_3ft.c
OBJECTS=xor_code.o xor_code_3ft.o
HEADERS=xor_code.h
CFLAGS=-O3 -msse3 
GCC=gcc

all: xor_test
  
xor_test: xor_code.c
	${GCC} ${CFLAGS} xor_code_3ft.c -o xor_code

clean:
	rm -rf xor_code *.o
