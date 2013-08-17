#include <emmintrin.h>  //SSE2
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

const int g_data_bit_lookup[] = {0x1, 0x2, 0x4, 0x8,
                                 0x10, 0x20, 0x40, 0x80,
                                 0x100, 0x200, 0x400, 0x800,
                                 0x1000, 0x2000, 0x4000, 0x8000,
                                 0x10000, 0x20000, 0x40000, 0x80000,
                                 0x100000, 0x200000, 0x400000, 0x800000,
                                 0x1000000, 0x2000000, 0x4000000, 0x8000000,
                                 0x10000000, 0x20000000, 0x40000000, 0x80000000};

int g_12_6_code_parity_bms[] = { 1649, 3235, 2375, 718, 1436, 2872 }; 

#define is_data_in_parity(data_idx, parity_bm) ((g_data_bit_lookup[data_idx] & parity_bm) == g_data_bit_lookup[data_idx])
#define is_aligned(x) (((unsigned long)x & (MEM_ALIGN_SIZE-1)) == 0)
#define num_unaligned_end(size) (size % MEM_ALIGN_SIZE)

typedef struct xor_code_s
{
  int k;
  int m;
  int *parity_bms;
} xor_code_t;


int parity_bit_lookup(xor_code_t *code_desc, int index)
{
  return g_data_bit_lookup[code_desc->k - index];
}

int data_bit_lookup(xor_code_t *code_desc, int index)
{
  return g_data_bit_lookup[index];
}

int missing_elements_bm(xor_code_t *code_desc, int *missing_elements, int (*bit_lookup_func)(xor_code_t *code_desc, int index))
{
  int i = 0;
  int bm = 0;

  while (missing_elements[i] > -1) {
    bm |= bit_lookup_func(code_desc, missing_elements[i]);
  } 

  return bm;
}

failure_pattern_t get_failure_pattern(xor_code_t *code_desc, int *missing_idxs)
{
  int i = 0;
  failure_pattern_t pattern = FAIL_PATTERN_0D_0P;

  while (missing_idxs[i] > -1) {
    switch(pattern) {
      case FAIL_PATTERN_0D_0P:
        pattern = (missing_idxs[i] < code_desc->k) ? FAIL_PATTERN_1D_0P : FAIL_PATTERN_0D_1P;
        break;
      case FAIL_PATTERN_1D_0P:
        pattern = (missing_idxs[i] < code_desc->k) ? FAIL_PATTERN_2D_0P : FAIL_PATTERN_1D_1P;
        break;
      case FAIL_PATTERN_2D_0P:
        pattern = (missing_idxs[i] < code_desc->k) ? FAIL_PATTERN_3D_0P : FAIL_PATTERN_2D_1P;
        break;
      case FAIL_PATTERN_3D_0P:
        pattern = FAIL_PATTERN_GE_HD; 
        break;
      case FAIL_PATTERN_1D_1P:
        pattern = (missing_idxs[i] < code_desc->k) ? FAIL_PATTERN_2D_1P : FAIL_PATTERN_1D_2P;
        break;
      case FAIL_PATTERN_1D_2P:
        pattern = FAIL_PATTERN_GE_HD; 
        break;
      case FAIL_PATTERN_2D_1P:
        pattern = FAIL_PATTERN_GE_HD; 
        break;
      case FAIL_PATTERN_0D_1P:
        pattern = (missing_idxs[i] < code_desc->k) ? FAIL_PATTERN_1D_1P : FAIL_PATTERN_0D_2P;
        break;
      case FAIL_PATTERN_0D_2P:
        pattern = (missing_idxs[i] < code_desc->k) ? FAIL_PATTERN_1D_2P : FAIL_PATTERN_0D_3P;
        break;
      case FAIL_PATTERN_0D_3P:
        pattern = FAIL_PATTERN_GE_HD; 
        break;
    } 
    if (pattern == FAIL_PATTERN_GE_HD) {
      break;
    }
    i++;
  }

  return pattern; 
}

void *aligned_malloc( size_t size, int align )
{
    void *mem = malloc( size + (align-1) + sizeof(void*) );
    char *amem;
    if (!mem) {
      return NULL;
    }

    amem = ((char*)mem) + sizeof(void*);
    amem += align - ((unsigned long)amem & (align - 1));

    ((void**)amem)[-1] = mem;
    return amem;
}

void aligned_free( void *mem )
{
    free( ((void**)mem)[-1] );
}

void fast_memcpy(char *dst, char *src, int size)
{
    // Use _mm_stream_si128((__m128i*) _buf2, sum);
    memcpy(dst, src, size);
}

/*
 * Buffers must be aligned to 16-byte boundaries
 *
 * Store in buf2 (opposite of memcpy convention...  Maybe change?)
 */
void xor_bufs_and_store(char *buf1, char *buf2, int blocksize)
{
  int residual_bytes = num_unaligned_end(blocksize);
  int fast_blocksize = blocksize > residual_bytes ? (blocksize - residual_bytes) : 0;
  int fast_int_blocksize = fast_blocksize / sizeof(__m128i);
  int i;
  __m128i *_buf1 = (__m128i*)buf1; 
  __m128i *_buf2 = (__m128i*)buf2; 
 
  /*
   * XOR aligned region using 128-bit XOR
   */
  for (i=0; i < fast_int_blocksize; i++) {
    _buf2[i] = _mm_xor_si128(_buf1[i], _buf2[i]);
  }

  /*
   * XOR unaligned end of region
   */
  for (i=fast_blocksize; i < blocksize; i++)
  {
    buf2[i] ^= buf1[i];
  }
}

void encode(xor_code_t *code_desc, char **data, char **parity, int blocksize)
{
  int i, j;
  
  for (i=0; i < code_desc->k; i++) {
    for (j=0; j < code_desc->m; j++) {
      if (is_data_in_parity(i, code_desc->parity_bms[j])) {
        xor_bufs_and_store(data[i], parity[j], blocksize);
      }
    }
  }
}

void selective_encode(xor_code_t *code_desc, char **data, char **parity, int *missing_parity, int blocksize)
{
  int i = 0, j;
  int missing_parity_bm = 0;

  while (missing_parity[i] > -1) {
    missing_parity_bm |= ((1 << code_desc->k-missing_parity[i]));
    i++;
  }
  
  for (i=0; i < code_desc->k; i++) {
    for (j=0; j < code_desc->m; j++) {
      if (is_data_in_parity(i, code_desc->parity_bms[j]) && (missing_parity_bm & (1 << j) == (1 << j))) {
        xor_bufs_and_store(data[i], parity[j], blocksize);
      }
    }
  }
}

int * get_missing_parity(xor_code_t *code_desc, int *missing_idxs)
{
  int *missing_parity = (int*)malloc(sizeof(int)*MAX_PARITY);
  int i = 0, j = 0;

  while (missing_idxs[i] > -1) {
    if (missing_idxs[i] >= code_desc->k) {
      missing_parity[j] = missing_idxs[i]; 
      j++;
    }
    i++;
  }
  
  missing_parity[j] = -1;
  return missing_parity;
}

int * get_missing_data(xor_code_t *code_desc, int *missing_idxs)
{
  int *missing_data = (int*)malloc(sizeof(int)*MAX_DATA);
  int i = 0, j = 0;

  while (missing_idxs[i] > -1) {
    if (missing_idxs[i] < code_desc->k) {
      missing_data[j] = missing_idxs[i]; 
      j++;
    }
    i++;
  }
  
  missing_data[j] = -1;
  return missing_data;
}

int index_of_connected_parity(xor_code_t *code_desc, int data_index, int *missing_parity)
{
  int parity_index = -1;
  int i;
  
  for (i=0; i < code_desc->m; i++) {
    if (is_data_in_parity(data_index, code_desc->parity_bms[i])) {
      int j=0;
      int is_missing = 0;
      if (missing_parity == NULL) {
        parity_index = i;
        break;
      }
      while (missing_parity[j] > -1) {
        if ((code_desc->k + i) == missing_parity[j]) {
          is_missing = 1; 
          break; 
        }
      }
      if (!is_missing) {
        parity_index = i;
        break;
      }
    }
  }
  
  // Must add k to get the absolute
  // index of the parity in the stripe
  return parity_index > -1 ? parity_index + code_desc->k : parity_index;
}

/*
 * There is one unavailable data element, so any available parity connected to
 * the data element is sufficient to decode.
 */
void decode_one_data(xor_code_t *code_desc, char **data, char **parity, int *missing_data, int *missing_parity, int blocksize)
{
  // Verify that missing_data[1] == -1? 
  int data_index = missing_data[0];
  int parity_index = index_of_connected_parity(code_desc, data_index, missing_parity);
  int i;

  // Copy the appropriate parity into the data buffer
  fast_memcpy(data[data_index], parity[parity_index-code_desc->k], blocksize);

  for (i=0; i < code_desc->k; i++) {
    if (i != data_index && is_data_in_parity(i, code_desc->parity_bms[parity_index-code_desc->k])) {
      xor_bufs_and_store(data[i], data[data_index], blocksize);
    }
  }
}

void decode_two_data(xor_code_t *code_desc, char **data, char **parity, int *missing_data, int *missing_parity, int blocksize)
{
  // Verify that missing_data[2] == -1?
  int data_index = missing_data[0];
  int parity_index = index_of_connected_parity(code_desc, data_index, missing_parity);
  int i;

  if (parity_index < 0) {
    data_index = missing_data[1];
    parity_index = index_of_connected_parity(code_desc, data_index, missing_parity);
    if (parity_index < 0) {
      fprintf(stderr, "Shit is broken, cannot find a proper parity!!!\n");
      exit(2);
    }
  } else {
    missing_data[0] = missing_data[1];
    missing_data[1] = -1;
  }
  
  // Copy the appropriate parity into the data buffer
  fast_memcpy(data[data_index], parity[parity_index-code_desc->k], blocksize);

  for (i=0; i < code_desc->k; i++) {
    if (i != data_index && is_data_in_parity(i, code_desc->parity_bms[parity_index-code_desc->k])) {
      xor_bufs_and_store(data[i], data[data_index], blocksize);
    }
  }
}

void decode_three_data(xor_code_t *code_desc, char **data, char **parity, int *missing_data, int *missing_parity, int blocksize)
{
}

void decode(xor_code_t *code_desc, char **data, char **parity, int *missing_idxs, int blocksize, int decode_parity)
{
  failure_pattern_t pattern = get_failure_pattern(code_desc, missing_idxs);

  switch(pattern) {
    case FAIL_PATTERN_0D_0P: 
      break;
    case FAIL_PATTERN_1D_0P: 
    {
      int *missing_data = get_missing_data(code_desc, missing_idxs);
      decode_one_data(code_desc, data, parity, missing_data, NULL, blocksize);
      free(missing_data);
      break;
    }
    case FAIL_PATTERN_2D_0P: 
    {
      int *missing_data = get_missing_data(code_desc, missing_idxs);
      decode_two_data(code_desc, data, parity, missing_data, NULL, blocksize);
      free(missing_data);
      break;
    }
    case FAIL_PATTERN_3D_0P: 
    {
      int *missing_data = get_missing_data(code_desc, missing_idxs);
      decode_three_data(code_desc, data, parity, missing_data, NULL, blocksize);
      free(missing_data);
      break;
    }
    case FAIL_PATTERN_1D_1P: 
    {
      int *missing_data = get_missing_data(code_desc, missing_idxs);
      int *missing_parity = get_missing_parity(code_desc, missing_idxs);
      decode_one_data(code_desc, data, parity, missing_data, missing_parity, blocksize);
      if (decode_parity) {
        int *missing_parity = get_missing_parity(code_desc, missing_idxs);
        selective_encode(code_desc, data, parity, missing_parity, blocksize);
        free(missing_parity);
      }
      free(missing_data);
      break;
    }
    case FAIL_PATTERN_1D_2P: 
    {
      int *missing_data = get_missing_data(code_desc, missing_idxs);
      int *missing_parity = get_missing_parity(code_desc, missing_idxs);
      decode_one_data(code_desc, data, parity, missing_data, missing_parity, blocksize);
      if (decode_parity) {
        int *missing_parity = get_missing_parity(code_desc, missing_idxs);
        selective_encode(code_desc, data, parity, missing_parity, blocksize);
        free(missing_parity);
      }
      free(missing_data);
      break;
    }
    case FAIL_PATTERN_2D_1P: 
    {
      int *missing_data = get_missing_data(code_desc, missing_idxs);
      int *missing_parity = get_missing_parity(code_desc, missing_idxs);
      decode_two_data(code_desc, data, parity, missing_data, missing_parity, blocksize);
      if (decode_parity) {
        int *missing_parity = get_missing_parity(code_desc, missing_idxs);
        selective_encode(code_desc, data, parity, missing_parity, blocksize);
        free(missing_parity);
      }
      free(missing_data);
      break;
    }
    case FAIL_PATTERN_0D_1P: 
      if (decode_parity) {
        int *missing_parity = get_missing_parity(code_desc, missing_idxs);
        selective_encode(code_desc, data, parity, missing_parity, blocksize);
        free(missing_parity);
      }
      break;
    case FAIL_PATTERN_0D_2P: 
      if (decode_parity) {
        int *missing_parity = get_missing_parity(code_desc, missing_idxs);
        selective_encode(code_desc, data, parity, missing_parity, blocksize);
        free(missing_parity);
      }
      break;
    case FAIL_PATTERN_0D_3P:
      if (decode_parity) {
        int *missing_parity = get_missing_parity(code_desc, missing_idxs);
        selective_encode(code_desc, data, parity, missing_parity, blocksize);
        free(missing_parity);
      }
      break;
  }

  return;
}

void fill_buffer(unsigned char *buf, int size, int seed)
{
  int i;

  buf[0] = seed;

  for (i=1; i < size; i++) {
    buf[i] = ((buf[i-1] + i) % 256);
  }
}

int check_buffer(unsigned char *buf, int size, int seed)
{
  int i;

  if (buf[0] != seed) {
    fprintf(stderr, "Seed does not match index 0: %u\n", buf[0]);
    return -1;
  }

  for (i=1; i < size; i++) {
    if (buf[i] != ((buf[i-1] + i) % 256)) {
      fprintf(stderr, "Buffer does not match index %d: %u\n", i, (buf[i] & 0xff));
      return -1;
    }
  }

  return 0;
}

int main()
{
  int i;
  int num_iter = 1000;
  int blocksize = 32768;
  int missing_idxs[4] = { -1 };
  char **data, **parity;
  xor_code_t *code_desc = (xor_code_t*)malloc(sizeof(xor_code_t));
  clock_t start_time, end_time;

  srand(time(NULL));

  code_desc->k = 12;
  code_desc->m = 6;
  code_desc->parity_bms = g_12_6_code_parity_bms;

  data = (char**)malloc(code_desc->k * sizeof(char*));
  parity = (char**)malloc(code_desc->m * sizeof(char*));

  for (i=0; i < code_desc->k; i++) {
    data[i] = aligned_malloc(blocksize, 16);
    fill_buffer(data[i], blocksize, i);
    if (!data[i]) {
      fprintf(stderr, "Could not allocate memnory for data %d\n", i);
      exit(2);
    }
  }
  
  for (i=0; i < code_desc->m; i++) {
    parity[i] = aligned_malloc(blocksize, 16);
    memset(parity[i], 0, blocksize);
    if (!parity[i]) {
      fprintf(stderr, "Could not allocate memnory for parity %d\n", i);
      exit(2);
    }
  }
  
  start_time = clock();
  for (i=0; i < num_iter-1; i++) {
    encode(code_desc, data, parity, blocksize);
  }
  end_time = clock();

  fprintf(stderr, "Encode: %.2f MB/s\n", ((double)(num_iter * blocksize * code_desc->k) / 1000 / 1000 ) / ((double)(end_time-start_time) / CLOCKS_PER_SEC));
  
  for (i=0; i < code_desc->m; i++) {
    memset(parity[i], 0, blocksize);
  }

  encode(code_desc, data, parity, blocksize);
  
  start_time = clock();
  for (i=0; i < num_iter; i++) {
    missing_idxs[0] = rand() % code_desc->k;
    missing_idxs[1] = -1;
    memset(data[missing_idxs[0]], 0, blocksize);
    decode(code_desc, data, parity, missing_idxs, blocksize, 0);
  
    if (check_buffer(data[missing_idxs[0]], blocksize, missing_idxs[0]) < 0) {
      fprintf(stderr, "Decode did not work!\n");
      exit(2);
    }
  }
  end_time = clock();
  
  fprintf(stderr, "Decode: %.2f MB/s\n", ((double)(num_iter * blocksize * code_desc->k) / 1000 / 1000 ) / ((double)(end_time-start_time) / CLOCKS_PER_SEC));

  return 0;
}

