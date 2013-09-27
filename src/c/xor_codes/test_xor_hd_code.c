#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <xor_code.h>
#include <test_xor_hd_code.h>

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

int test_hd_code(xor_code_t *code_desc, int num_failure_combs, int failure_combs[][4])
{
  int i;
  int num_iter = 1000;
  int blocksize = 32768;
  int missing_idxs[4] = { -1 };
  char **data, **parity;
  clock_t start_time, end_time;

  srand(time(NULL));

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
  
  for (i=0; i < num_failure_combs; i++) {
    int missing_idx_0 = failure_combs[i][0];
    int missing_idx_1 = failure_combs[i][1];
    int missing_idx_2 = failure_combs[i][2];

    missing_idxs[0] = missing_idx_0;
    missing_idxs[1] = missing_idx_1;
    missing_idxs[2] = missing_idx_2;
    missing_idxs[3] = -1;

    if (missing_idxs[0] > -1) {
      if (missing_idxs[0] < code_desc->k) {
        memset(data[missing_idxs[0]], 0, blocksize);
      } else {
        memset(parity[missing_idxs[0] - code_desc->k], 0, blocksize);
      }
    }
    if (missing_idxs[1] > -1) {
      if (missing_idxs[1] < code_desc->k) {
        memset(data[missing_idxs[1]], 0, blocksize);
      } else {
        memset(parity[missing_idxs[1] - code_desc->k], 0, blocksize);
      }
    }
    if (missing_idxs[2] > -1) {
      if (missing_idxs[2] < code_desc->k) {
        memset(data[missing_idxs[2]], 0, blocksize);
      } else {
        memset(parity[missing_idxs[2] - code_desc->k], 0, blocksize);
      }
    }

    code_desc->decode(code_desc, data, parity, missing_idxs, blocksize, 1);
  
    if (missing_idxs[0] > -1 && missing_idxs[0] < code_desc->k && check_buffer(data[missing_idx_0], blocksize, missing_idx_0) < 0) {
      fprintf(stderr, "Decode did not work: %d (%d %d %d)!\n", missing_idxs[0], missing_idxs[0], missing_idxs[1], missing_idxs[2]);
      exit(2);
    }
    if (missing_idxs[1] > -1 && missing_idxs[1] < code_desc->k && check_buffer(data[missing_idx_1], blocksize, missing_idx_1) < 0) {
      fprintf(stderr, "Decode did not work: %d (%d %d %d)!\n", missing_idxs[1], missing_idxs[0], missing_idxs[1], missing_idxs[2]);
      exit(2);
    }
    if (missing_idxs[2] > -1 && missing_idxs[2] < code_desc->k && check_buffer(data[missing_idx_2], blocksize, missing_idx_2) < 0) {
      fprintf(stderr, "Decode did not work: %d (%d %d %d)!\n", missing_idxs[2], missing_idxs[0], missing_idxs[1], missing_idxs[2]);
      exit(2);
    }
  }

  start_time = clock();
  for (i=0; i < num_iter; i++) {
    int j;

    missing_idxs[0] = rand() % (code_desc->k + code_desc->m);
    for (j=1; j < code_desc->hd-1;j++) {
      missing_idxs[j] = (missing_idxs[j-1] + 1) % (code_desc->k + code_desc->m);
    }
    missing_idxs[code_desc->hd-1] = -1;

    if (missing_idxs[0] > -1 && missing_idxs[0] < code_desc->k) {
      memset(data[missing_idxs[0]], 0, blocksize);
    }
    if (missing_idxs[1] > -1 && missing_idxs[1] < code_desc->k) {
      memset(data[missing_idxs[1]], 0, blocksize);
    }
    if (missing_idxs[2] > -1 && missing_idxs[2] < code_desc->k) {
      memset(data[missing_idxs[2]], 0, blocksize);
    }

    code_desc->decode(code_desc, data, parity, missing_idxs, blocksize, 1);
  }
  end_time = clock();
  
  fprintf(stderr, "Decode: %.2f MB/s\n", ((double)(num_iter * blocksize * code_desc->k) / 1000 / 1000 ) / ((double)(end_time-start_time) / CLOCKS_PER_SEC));

  return 0;
}

int run_test(int k, int m, int hd)
{
  int ret = -1;
  xor_code_t * code_desc = init_xor_hd_code(k, m, hd);

  fprintf(stderr, "Running (%d, %d, %d):\n", k, m, hd);
  
  switch(k+m) {
    case 10:
      if (hd == 3) {
        ret = test_hd_code(code_desc, NUM_10_3_COMBS, failure_combs_10_3);
      } else {
        ret = test_hd_code(code_desc, NUM_10_4_COMBS, failure_combs_10_4);
      }
      break;
    case 11:
      if (hd == 3) {
        ret = test_hd_code(code_desc, NUM_11_3_COMBS, failure_combs_11_3);
      } else {
        ret = test_hd_code(code_desc, NUM_11_4_COMBS, failure_combs_11_4);
      }
      break;
    case 12:
      if (hd == 3) {
        ret = test_hd_code(code_desc, NUM_12_3_COMBS, failure_combs_12_3);
      } else {
        ret = test_hd_code(code_desc, NUM_12_4_COMBS, failure_combs_12_4);
      }
      break;
    case 13:
      if (hd == 3) {
        ret = test_hd_code(code_desc, NUM_13_3_COMBS, failure_combs_13_3);
      } else {
        ret = test_hd_code(code_desc, NUM_13_4_COMBS, failure_combs_13_4);
      }
      break;
    case 14:
      if (hd == 3) {
        ret = test_hd_code(code_desc, NUM_14_3_COMBS, failure_combs_14_3);
      } else {
        ret = test_hd_code(code_desc, NUM_14_4_COMBS, failure_combs_14_4);
      }
      break;
    case 15:
      if (hd == 3) {
        ret = test_hd_code(code_desc, NUM_15_3_COMBS, failure_combs_15_3);
      } else {
        ret = test_hd_code(code_desc, NUM_15_4_COMBS, failure_combs_15_4);
      }
      break;
    case 16:
      if (hd == 3) {
        ret = test_hd_code(code_desc, NUM_16_3_COMBS, failure_combs_16_3);
      } else {
        ret = test_hd_code(code_desc, NUM_16_4_COMBS, failure_combs_16_4);
      }
      break;
    case 17:
      if (hd == 3) {
        ret = test_hd_code(code_desc, NUM_17_3_COMBS, failure_combs_17_3);
      } else {
        ret = test_hd_code(code_desc, NUM_17_4_COMBS, failure_combs_17_4);
      }
      break;
    case 18:
      if (hd == 3) {
        ret = test_hd_code(code_desc, NUM_18_3_COMBS, failure_combs_18_3);
      } else {
        ret = test_hd_code(code_desc, NUM_18_4_COMBS, failure_combs_18_4);
      }
      break;
    case 19:
      if (hd == 3) {
        ret = test_hd_code(code_desc, NUM_19_3_COMBS, failure_combs_19_3);
      } else {
        ret = test_hd_code(code_desc, NUM_19_4_COMBS, failure_combs_19_4);
      }
      break;
    case 20:
      if (hd == 3) {
        ret = test_hd_code(code_desc, NUM_20_3_COMBS, failure_combs_20_3);
      } else {
        ret = test_hd_code(code_desc, NUM_20_4_COMBS, failure_combs_20_4);
      }
      break;
    case 21:
      if (hd == 3) {
        ret = test_hd_code(code_desc, NUM_21_3_COMBS, failure_combs_21_3);
      } else {
        ret = test_hd_code(code_desc, NUM_21_4_COMBS, failure_combs_21_4);
      }
      break;
    case 22:
      ret = test_hd_code(code_desc, NUM_22_4_COMBS, failure_combs_22_4);
      break;
    case 23:
      ret = test_hd_code(code_desc, NUM_23_4_COMBS, failure_combs_23_4);
      break;
    case 24:
      ret = test_hd_code(code_desc, NUM_24_4_COMBS, failure_combs_24_4);
      break;
    case 25:
      ret = test_hd_code(code_desc, NUM_25_4_COMBS, failure_combs_25_4);
      break;
    case 26:
      ret = test_hd_code(code_desc, NUM_26_4_COMBS, failure_combs_26_4);
      break;
    default:
      ret = -1; 
  }
  free(code_desc);
  return ret; 
}

int main()
{
  int ret = 0;
  int i;
  
  for (i=6; i < 16; i++) {
    ret = run_test(i, 6, 3);
    if (ret != 0) {
      return ret;
    }
  }
  
  for (i=5; i < 11; i++) {
    ret = run_test(i, 5, 3);
    if (ret != 0) {
      return ret;
    }
  }
  
  for (i=6; i < 21; i++) {
    ret = run_test(i, 6, 4);
    if (ret != 0) {
      return ret;
    }
  }
  
  for (i=5; i < 11; i++) {
    ret = run_test(i, 5, 4);
    if (ret != 0) {
      return ret;
    }
  }
}

