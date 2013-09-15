#ifndef __PYEC_LIB_H_
#define __PYEC_LIB_H_

typedef enum { PYECC_RS_VAND, PYECC_RS_CAUCHY_ORIG, PYECC_NUM_TYPES, PYECC_NOT_FOUND } pyeclib_type_t;

const char *pyeclib_type_str[] = { "rs_vand", "rs_cauchy_orig" };
const int pyeclib_type_word_size_bytes[] = { 2, sizeof(int) };

#define PYECC_FLAGS_MASK          0x1
#define PYECC_FLAGS_READ_VERIFY   0x1
#define PYECC_HANDLE_NAME "pyeclib_handle"
#define PYECC_HEADER_MAGIC  0xb0c5ecc
#define PYECC_CAUCHY_PACKETSIZE sizeof(long) * 16

/*
 * Prevent the compiler from padding
 * this by using the __packed__ keyword
 */
typedef struct __attribute__((__packed__)) fragment_header_s
{
  int magic;
  int idx;
  int size;
  int stripe_padding;
  int fragment_padding;
} fragment_header_t;

typedef struct pyeclib_s
{
  int k;
  int m;
  int w;
  int *matrix;
  int *bitmatrix;
  int **schedule;
  pyeclib_type_t type;
} pyeclib_t;

#define FRAGSIZE_2_BLOCKSIZE(fragment_size) (fragment_size - sizeof(fragment_header_t))

#define PYECLIB_WORD_SIZE(type) pyeclib_type_word_size_bytes[type]

#endif

