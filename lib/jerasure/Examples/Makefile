# Examples/makefile
# James S. Plank
# 
# JERASURE - Library for Erasure Coding
# Copright (C) 2007 James S. Plank
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
# 
# James S. Plank
# Department of Computer Science
# University of Tennessee
# Knoxville, TN 37996
# plank@cs.utk.edu

# $Revision: 1.2 $
# $Date: 2008/08/19 17:41:40 $

CC = gcc
CFLAGS = -g -I../include
LDFLAGS = -L../lib -lJerasure

ALL =	jerasure_01 \
        jerasure_02 \
        jerasure_03 \
        jerasure_04 \
        jerasure_05 \
        jerasure_06 \
        jerasure_07 \
        jerasure_08 \
        reed_sol_01 \
        reed_sol_02 \
        reed_sol_03 \
        reed_sol_04 \
        cauchy_01 \
        cauchy_02 \
        cauchy_03 \
        cauchy_04 \
        liberation_01 \
		encoder \
		decoder \

all: $(ALL)

clean:
	rm -f core *.o $(ALL) test_file.0

../lib/libJerasure.so:
	make -C ../src/

jerasure_01: jerasure_01.c ../lib/libJerasure.so
	$(CC) $(CFLAGS) -o jerasure_01 jerasure_01.c $(LDFLAGS)

jerasure_02: jerasure_02.c ../lib/libJerasure.so
	$(CC) $(CFLAGS)  -o jerasure_02 jerasure_02.c $(LDFLAGS)

jerasure_03: jerasure_03.c 
	$(CC) $(CFLAGS)  -o jerasure_03 jerasure_03.c $(LDFLAGS)

jerasure_04: jerasure_04.c 
	$(CC) $(CFLAGS)  -o jerasure_04 jerasure_04.c $(LDFLAGS)

jerasure_05: jerasure_05.c
	$(CC) $(CFLAGS)  -o jerasure_05 jerasure_05.c $(LDFLAGS)

jerasure_06: jerasure_06.c
	$(CC) $(CFLAGS)  -o jerasure_06 jerasure_06.c $(LDFLAGS)

jerasure_07: jerasure_07.c
	$(CC) $(CFLAGS)  -o jerasure_07 jerasure_07.c $(LDFLAGS)

jerasure_08: jerasure_08.c
	$(CC) $(CFLAGS)  -o jerasure_08 jerasure_08.c $(LDFLAGS)

reed_sol_01: reed_sol_01.c
	$(CC) $(CFLAGS)  -o reed_sol_01 reed_sol_01.c $(LDFLAGS)

reed_sol_02: reed_sol_02.c
	$(CC) $(CFLAGS)  -o reed_sol_02 reed_sol_02.c $(LDFLAGS)

reed_sol_03: reed_sol_03.c
	$(CC) $(CFLAGS)  -o reed_sol_03 reed_sol_03.c $(LDFLAGS)

reed_sol_04: reed_sol_04.c
	$(CC) $(CFLAGS)  -o reed_sol_04 reed_sol_04.c $(LDFLAGS)

cauchy_01: cauchy_01.c
	$(CC) $(CFLAGS)  -o cauchy_01 cauchy_01.c $(LDFLAGS)

cauchy_02: cauchy_02.c
	$(CC) $(CFLAGS)  -o cauchy_02 cauchy_02.c $(LDFLAGS)

cauchy_03: cauchy_03.c
	$(CC) $(CFLAGS)  -o cauchy_03 cauchy_03.c $(LDFLAGS)

cauchy_04: cauchy_04.c
	$(CC) $(CFLAGS)  -o cauchy_04 cauchy_04.c $(LDFLAGS)

liberation_01: liberation_01.c
	$(CC) $(CFLAGS)  -o liberation_01 liberation_01.c $(LDFLAGS)

encoder: encoder.c
	$(CC) $(CFLAGS)  -o encoder encoder.c $(LDFLAGS)

decoder: decoder.c
	$(CC) $(CFLAGS)  -o decoder decoder.c $(LDFLAGS)

test_file: test_file.0
