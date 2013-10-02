/* * Copyright (c) 2013, Kevin Greenan (kmgreen2@gmail.com)
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

#include<alg_sig.h>
#include<stdlib.h>
#include<string.h>

int valid_gf_w[] = { 16, -1 };
int prim_poly[] = { 0210013, -1 };

alg_sig_t *init_alg_sig(int sig_len, int gf_w)
{
    alg_sig_t *alg_sig_handle;
    int num_gf_lr_table_syms;
    int i;
    int gf_idx;

    i=0;
    while (valid_gf_w[i] > -1) {
      if (gf_w == valid_gf_w[i]) {
        break;
      }
      i++;
    }

    if (valid_gf_w[i] == -1) {
      return NULL;
    }

    gf_idx = i;

    alg_sig_handle = (alg_sig_t *)malloc(sizeof(alg_sig_t));
    if (alg_sig_handle == NULL) {
      return NULL;
    }

    alg_sig_handle->sig_len = sig_len;
    alg_sig_handle->gf_w = gf_w;

    num_gf_lr_table_syms = 1 << (gf_w >> 1);

    alg_sig_handle->tbl1 = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
    alg_sig_handle->tbl2 = (int*)malloc(sizeof(int) * num_gf_lr_table_syms);
    
    /*
     * Note that \alpha = 2 
     */
    for (i = 0; i < 256; i++) {
      alg_sig_handle->tbl1[i] = galois_single_multiply((unsigned short) (i << 8), 2, gf_w);
      alg_sig_handle->tbl2[i] = galois_single_multiply((unsigned short) i, 2, gf_w);
    }

    return alg_sig_handle;
}

int compute_alg_sig32(alg_sig_t *alg_sig_handle, char *buf, int len, char *sig)
{
  int bit_mask;
  int adj_len = len / 2;
  int i;
  unsigned short *_buf = (unsigned short *)buf;
  unsigned short sig_buf[2];

  if (len == 0) {
    bzero(sig, 8);
    return;
  }

  switch (len % 2) {
      case 1:
          bit_mask = 0x00ff;
          break;
      default:
          bit_mask = 0xffff;
          break;
  }

  if (len % 2 > 0) {
      adj_len++;
  }

  // Account for buffer not being uint16_t aligned
  sig_buf[0] = (_buf[adj_len - 1] & bit_mask);
  sig_buf[1] = (_buf[adj_len - 1] & bit_mask);

  /**
   * This is the loop to optimize.  It is currently optimized enough : using Horner's alg.,
   * shortened mult. tables, and other tricks.
   */
  for (i = adj_len - 2; i >= 0; i--) {
      sig_buf[0] ^= _buf[i];
      sig_buf[1] = (_buf[i] ^ (alg_sig_handle->tbl1[(sig_buf[1] >> 8) & 0x00ff] ^ alg_sig_handle->tbl2[sig_buf[1] & 0x00ff]));
  }

  sig[0] = (char) (sig_buf[0] & 0x000ff);
  sig[1] = (char) ((sig_buf[0] >> 8) & 0x000ff);
  sig[2] = (char) (sig_buf[1] & 0x00ff);
  sig[3] = (char) ((sig_buf[1] >> 8) & 0x00ff);
  return 0;
}

