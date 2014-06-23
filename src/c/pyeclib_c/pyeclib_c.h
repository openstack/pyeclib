/* 
 * Copyright (c) 2013-2014, Kevin Greenan (kmgreen2@gmail.com)
 * Copyright (c) 2014, Tushar Gohad (tusharsg@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.  THIS SOFTWARE IS PROVIDED BY
 * THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __PYEC_LIB_C_H_
#define __PYEC_LIB_C_H_

/* 
 * Make sure these enum values match those exposed from the Python EC interface
 * src/python/pyeclib/ec_iface.py
 */
#define PYECLIB_MAX_DATA              32
#define PYECLIB_MAX_PARITY            32

typedef enum {
    PYECC_NOT_FOUND = 0,
    PYECC_RS_VAND = 1, 
    PYECC_RS_CAUCHY_ORIG = 2,
    PYECC_XOR_HD_3 = 3, 
    PYECC_XOR_HD_4 = 4, 
    PYECC_NUM_TYPES = 4, 
} pyeclib_type_t;

const char *pyeclib_type_str[] = { 
    "not_found",
    "rs_vand", 
    "rs_cauchy_orig", 
    "flat_xor_3",
    "flat_xor_4", 
};

const int pyeclib_type_word_size_bytes[] = { 
    0,
    sizeof(long),   /* rs_vand */
    sizeof(long),   /* rs_cauchy_orig */
    sizeof(long),   /* flat_xor_3 */
    sizeof(long)    /* flat_xor_4 */
};

const int pyeclib_type_best_w_size_bytes[] = { 
    32,             /* default */
    16,             /* rs_vand */
     4,             /* rs_cauchy_orig */
    32,             /* flat_xor_3 */
    32              /* flat_xor_4 */
};

// Unconditionally enforce alignment for now.
// This is needed for the SIMD extentions.
// TODO (kmg): Parse cpuinfo and determine if it is necessary...
const int pyeclib_type_needs_addr_align[] = { 
    -1, 
     1, 
     1, 
     1, 
     1 
};

typedef struct pyeclib_s
{
  int k;
  int m;
  int w;
  int *matrix;
  int *bitmatrix;
  int **schedule;
  xor_code_t *xor_code_desc;
  alg_sig_t  *alg_sig_desc;
  pyeclib_type_t type;
  int inline_chksum;
  int algsig_chksum;
} pyeclib_t;

/*
 * Convert the string ECC type to the enum value
 * Start lookup at index 1
 */
static inline
pyeclib_type_t get_ecc_type(const char *str_type)
{
  int i;
  for (i = 1; i <= PYECC_NUM_TYPES; i++) {
    if (strcmp(str_type, pyeclib_type_str[i]) == 0) {
      return i; 
    }
  }
  return PYECC_NOT_FOUND;
}

/*
 * Convert the string ECC type to the w value
 * Start lookup at index 1
 */
static inline
unsigned int get_best_w_for_ecc_type(pyeclib_type_t type)
{
    if (type > PYECC_NUM_TYPES)
        return -1;

    return pyeclib_type_best_w_size_bytes[type];
}


#define PYECC_FLAGS_MASK            0x1
#define PYECC_FLAGS_READ_VERIFY     0x1
#define PYECC_HANDLE_NAME           "pyeclib_handle"
#define PYECC_HEADER_MAGIC          0xb0c5ecc
#define PYECC_CAUCHY_PACKETSIZE     (sizeof(long) * 128)

#define PYCC_MAX_SIG_LEN            8

/*
 * Prevent the compiler from padding
 * this by using the __packed__ keyword
 */
typedef struct __attribute__((__packed__)) fragment_header_s
{
  int magic;
  int idx;
  int size;
  int orig_data_size;
  int chksum;
  // We must be aligned to 16-byte boundaries
  // So, size this array accordingly
  int aligned_padding[3];
} fragment_header_t;

typedef struct fragment_metadata_s
{
  int  size;
  int  idx;
  char signature[PYCC_MAX_SIG_LEN];
  int chksum_mismatch;
} fragment_metadata_t;

#define FRAGSIZE_2_BLOCKSIZE(fragment_size) (fragment_size - sizeof(fragment_header_t))

#define PYECLIB_WORD_SIZE(type) pyeclib_type_word_size_bytes[type]

#define PYECLIB_NEEDS_ADDR_ALIGNMENT(type) pyeclib_type_needs_addr_align[type]

#endif

