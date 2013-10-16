CFLAGS=-g -fPIC
GCC=gcc
INCLUDE_DIRS=-I./ -I../include -I/usr/local/include/jerasure
LIB_DIRS=-L/usr/local/lib
LIBS=-lJerasure

all: alg_sig_test

alg_sig_test: alg_sig.o
	${GCC} ${CFLAGS} ${INCLUDE_DIRS} ${LIB_DIRS} alg_sig.o alg_sig_test.c -o alg_sig_test ${LIBS}

alg_sig.o: alg_sig.c
	${GCC} ${CFLAGS} ${INCLUDE_DIRS} -c alg_sig.c 

clean:
	rm -rf alg_sig_test *.o
