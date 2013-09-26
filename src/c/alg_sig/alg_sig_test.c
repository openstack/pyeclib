#include<alg_sig.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<time.h>

#define SIG_LEN 32

void fill_random_buffer(char *buf, int size)
{
  int i;
  srand(time(NULL));
  
  for (i=0; i < size; i++) {
    buf[i] = (char)(rand() % 256);
  }
}

void compute_parity(char **data, char* parity, int num_data, int size)
{
  int i, j;

  bzero(parity, size);

  for (i=0; i < num_data; i++) {
    for (j=0; j < size; j++) {
      parity[j] ^= data[i][j];
    }
  }
}

int check_parity_of_sigs(char **sigs, int num_data, int size)
{
  int i, j;
  int ret = 0;
  char *parity_sig = (char*)malloc(SIG_LEN);

  bzero(parity_sig, SIG_LEN);

  for (i=0; i < num_data; i++) {
    for (j=0; j < SIG_LEN; j++) {
      parity_sig[j] ^= sigs[i][j];
    }
  } 

  if (memcmp(parity_sig, sigs[num_data], SIG_LEN) != 0) {
    fprintf(stderr, "Signatures do not match:\n");
    for (i=0; i < SIG_LEN; i++) {
      fprintf(stderr, "parity_sig[%d] = 0x%x, sigs[%d][%d] = 0x%x\n", i, parity_sig[i], num_data, i, sigs[i][num_data]);
    }
    ret = 1;
  }

out:
  free(parity_sig);
}

static int basic_xor_test()
{
  alg_sig_t* sig_handle = init_alg_sig(SIG_LEN, 16);
  int blocksize = 65536;
  int num_data = 12;
  char **data;
  char *parity;
  char **sigs;
  int i;
  int ret = 0;

  data = (char**)malloc(sizeof(char*) * num_data);
  sigs = (char**)malloc(sizeof(char*) * (num_data + 1));
  for (i=0; i < num_data; i++) {
    data[i] = (char*)malloc(sizeof(char)*blocksize);
    fill_random_buffer(data[i], blocksize);
    sigs[i] = (char*)malloc(SIG_LEN);
    
  }
  parity = (char*)malloc(sizeof(char)*blocksize);
  sigs[i] = (char*)malloc(SIG_LEN);

  compute_parity(data, parity, num_data, blocksize);

  for (i=0; i < num_data; i++) {
    compute_alg_sig32(sig_handle, data[i], blocksize, sigs[i]);
  }
  compute_alg_sig32(sig_handle, parity, blocksize, sigs[i]);
  
  ret = check_parity_of_sigs(sigs, num_data, blocksize);

  for (i=0; i < num_data; i++) {
    free(data[i]);
    free(sigs[i]);
  }

  free(parity);
  free(sigs[num_data]);
  free(sigs);
  free(data);

  return ret;
}

int main(int argc, char**argv)
{
  int ret;
  int num_failed = 0;

  ret = basic_xor_test();
  if (ret) {
    fprintf(stderr, "basic_xor_test has failed!\n"); 
    num_failed++;
  }

  if (num_failed == 0) {
    fprintf(stderr, "Tests pass!!!\n");
  }
}

