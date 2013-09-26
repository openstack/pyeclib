#ifndef _ALG_SIG_H
#define _ALG_SIG_H

#include<galois.h>

typedef struct alg_sig_s
{
  int gf_w;
  int sig_len;
  int *tbl1;
  int *tbl2;
} alg_sig_t;

alg_sig_t *init_alg_sig(int sig_len, int gf_w);

int compute_alg_sig32(alg_sig_t* alg_sig_handle, char *buf, int len, char *sig);

#endif

