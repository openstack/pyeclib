#ifndef _XOR_CODE_H
#define _XOR_CODE_H

#define MAX_DATA 32
#define MAX_PARITY MAX_DATA

#define MEM_ALIGN_SIZE 16

#define DECODED_MISSING_IDX MAX_DATA

typedef enum { FAIL_PATTERN_GE_HD, // Num failures greater than or equal to HD
               FAIL_PATTERN_0D_0P, 
               FAIL_PATTERN_1D_0P, 
               FAIL_PATTERN_2D_0P, 
               FAIL_PATTERN_3D_0P, 
               FAIL_PATTERN_1D_1P, 
               FAIL_PATTERN_1D_2P, 
               FAIL_PATTERN_2D_1P, 
               FAIL_PATTERN_0D_1P, 
               FAIL_PATTERN_0D_2P, 
               FAIL_PATTERN_0D_3P } failure_pattern_t;

#define is_aligned(x) (((unsigned long)x & (MEM_ALIGN_SIZE-1)) == 0)
#define num_unaligned_end(size) (size % MEM_ALIGN_SIZE)

struct xor_code_s;

typedef struct xor_code_s
{
  int k;
  int m;
  int hd;
  int *parity_bms;
  int *data_bms;
  void (*decode)(struct xor_code_s *code_desc, char **data, char **parity, int *missing_idxs, int blocksize, int decode_parity);
  void (*encode)(struct xor_code_s *code_desc, char **data, char **parity, int blocksize);
} xor_code_t;

int is_data_in_parity(int data_idx, int parity_bm);

int does_parity_have_data(int parity_idx, int data_bm);

int parity_bit_lookup(xor_code_t *code_desc, int index);

int data_bit_lookup(xor_code_t *code_desc, int index);

int missing_elements_bm(xor_code_t *code_desc, int *missing_elements, int (*bit_lookup_func)(xor_code_t *code_desc, int index));

failure_pattern_t get_failure_pattern(xor_code_t *code_desc, int *missing_idxs);

void *aligned_malloc( size_t size, int align);

void aligned_free(void *mem);

void fast_memcpy(char *dst, char *src, int size);

void xor_bufs_and_store(char *buf1, char *buf2, int blocksize);

void xor_code_encode(xor_code_t *code_desc, char **data, char **parity, int blocksize);

void selective_encode(xor_code_t *code_desc, char **data, char **parity, int *missing_parity, int blocksize);

int * get_missing_parity(xor_code_t *code_desc, int *missing_idxs);

int * get_missing_data(xor_code_t *code_desc, int *missing_idxs);

int num_missing_data_in_parity(xor_code_t *code_desc, int parity_idx, int *missing_data);

int index_of_connected_parity(xor_code_t *code_desc, int data_index, int *missing_parity, int *missing_data);

void remove_from_missing_list(int element, int *missing_list);

xor_code_t* init_xor_hd_code(int k, int m, int hd);

#endif
