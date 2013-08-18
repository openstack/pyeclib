#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <xor_code.h>

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

int test_12_6_4()
{
  int i;
  int num_iter = 1000;
  int blocksize = 32768;
  int missing_idxs[4] = { -1 };
  char **data, **parity;
  xor_code_t *code_desc; 
  clock_t start_time, end_time;


  srand(time(NULL));
  
  code_desc = init_xor_hd_code(12, 6, 4);

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
    code_desc->encode(code_desc, data, parity, blocksize);
  }
  end_time = clock();

  fprintf(stderr, "Encode: %.2f MB/s\n", ((double)(num_iter * blocksize * code_desc->k) / 1000 / 1000 ) / ((double)(end_time-start_time) / CLOCKS_PER_SEC));
  
  for (i=0; i < code_desc->m; i++) {
    memset(parity[i], 0, blocksize);
  }

  code_desc->encode(code_desc, data, parity, blocksize);
  
  start_time = clock();
  for (i=0; i < num_iter; i++) {
    int missing_idx_0 = rand() % code_desc->k;
    int missing_idx_1 = (missing_idx_0 + 2) % code_desc->k;
    int missing_idx_2 = (missing_idx_1 + 3) % code_desc->k;

    missing_idxs[0] = missing_idx_0;
    missing_idxs[1] = missing_idx_1;
    missing_idxs[2] = missing_idx_2;
    missing_idxs[3] = -1;

    memset(data[missing_idxs[0]], 0, blocksize);
    memset(data[missing_idxs[1]], 0, blocksize);
    memset(data[missing_idxs[2]], 0, blocksize);

    code_desc->decode(code_desc, data, parity, missing_idxs, blocksize, 0);
  
    if (check_buffer(data[missing_idx_0], blocksize, missing_idx_0) < 0) {
      fprintf(stderr, "Decode did not work!\n");
      exit(2);
    }
    if (check_buffer(data[missing_idx_1], blocksize, missing_idx_1) < 0) {
      fprintf(stderr, "Decode did not work!\n");
      exit(2);
    }
    if (check_buffer(data[missing_idx_2], blocksize, missing_idx_2) < 0) {
      fprintf(stderr, "Decode did not work!\n");
      exit(2);
    }
  }
  end_time = clock();
  
  fprintf(stderr, "Decode: %.2f MB/s\n", ((double)(num_iter * blocksize * code_desc->k) / 1000 / 1000 ) / ((double)(end_time-start_time) / CLOCKS_PER_SEC));

  return 0;
}

int main()
{
  return test_12_6_4();
}

