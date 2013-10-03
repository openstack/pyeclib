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

#include<Python.h>
#include<xor_code.h>
#include<reed_sol.h>
#include<cauchy.h>
#include<jerasure.h>
#include<liberation.h>
#include<galois.h>
#include<math.h>
#include<pyeclib_c.h>

/*
 * TODO (kmg): Cauchy restriction (k*w*PACKETSIZE)  < data_len / k, otherwise you could
 * end up with full-blocks of padding
 *
 */

/*
 * TODO (kmg): Do a big cleanup: standardize naming, types (typedef stuff), comments, standard error
 * handling and refactor where needed.
 */

/*
 * TODO (kmg): Need to clean-up all of the reference counting related stuff.  That is, INCREF before 
 * calling a function that will "steal" a reference and create functions to DECREF stuff.
 */

/*
 * Make the buffer alignment code in 'encode' generic and simple...  It sucks right now.
 */

static PyObject *PyECLibError;

/*
 * Determine if an address is aligned to a particular boundary
 */
static int is_addr_aligned(unsigned int addr, int align)
{
  return (addr & (align-1)) == 0;
}

/*
 * Validate the basic erasure code parameters 
 */
static int validate_args(int k, int m, int w, pyeclib_type_t type)
{

  switch (type) {
    case PYECC_RS_CAUCHY_ORIG:
      {
        if (w < 31 && (k+m) > (1 << w)) {
          return 0;
        }
      }
    case PYECC_XOR_HD_4:
    case PYECC_XOR_HD_3:
      return 1; 
    case PYECC_RS_VAND:
    default:
      {
        int max_symbols;
      
        if (w != 8 && w != 16 && w != 32) {
          return 0;
        }
        max_symbols = 1 << w;
        if ((k+m) > max_symbols) {
          return 0;
        }
      }
  }
  return 1;
}

/*
 * Convert an int list into a bitmap
 * Assume the list is '-1' terminated.
 */
static unsigned long convert_list_to_bitmap(int *list)
{
  int i = 0;
  unsigned long bm = 0;

  while (list[i] > -1) {
    /*
     * TODO: Assert list[i] < 64
     */
    bm |= (1 << list[i]);  
    i++;
  } 

  return bm;
}

/*
 * Convert the string ECC type to the enum value
 */
static pyeclib_type_t get_ecc_type(const char *str_type)
{
  int i;
  for (i=0; i < PYECC_NUM_TYPES; i++) {
    if (strcmp(str_type, pyeclib_type_str[i]) == 0) {
      return i; 
    }
  }
  return PYECC_NOT_FOUND;
}

static
void init_fragment_header(char *buf)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  header->magic = PYECC_HEADER_MAGIC;
}

static
char *alloc_fragment_buffer(int size)
{
  char *buf;
  fragment_header_t* header = NULL;

  size += sizeof(fragment_header_t);

  /*
   * Ensure all memory is aligned to
   * 16-byte boundaries to support 
   * 128-bit operations
   */
  if (posix_memalign((void**)&buf, 16, size) < 0) {
    return NULL;
  }
  bzero(buf, size);

  header = (fragment_header_t*)buf;
  header->magic = PYECC_HEADER_MAGIC;

  return buf;
}

static
char* get_fragment_data(char *buf)
{
  buf += sizeof(fragment_header_t);
  
  return buf;
}

static
char* get_fragment_ptr_from_data_novalidate(char *buf)
{
  buf -= sizeof(fragment_header_t);

  return buf;
}

static
char* get_fragment_ptr_from_data(char *buf)
{
  fragment_header_t *header;

  buf -= sizeof(fragment_header_t);

  header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    fprintf(stderr, "Invalid fragment header (get header ptr)!\n");
    return NULL; 
  }

  return buf;
}

static
int set_fragment_idx(char *buf, int idx)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    fprintf(stderr, "Invalid fragment header (idx check)!\n");
    return -1; 
  }

  header->idx = idx;
  
  return 0;
}

static
int get_fragment_idx(char *buf)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    PyErr_SetString(PyECLibError, "Invalid fragment header (get idx)!");
    return -1;
  }

  return header->idx;
  
  return 0;
}

static
int set_fragment_size(char *buf, int size)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    PyErr_SetString(PyECLibError, "Invalid fragment header (size check)!");
    return -1; 
  }

  header->size = size;
    
  return 0;
}

static
int get_fragment_size(char *buf)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    PyErr_SetString(PyECLibError, "Invalid fragment header (get size)!");
    return -1;
  }

  return header->size;
  
  return 0;
}

static
int set_stripe_padding(char *buf, int stripe_padding)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    PyErr_SetString(PyECLibError, "Invalid fragment header (stripe padding check)!");
    return -1; 
  }

  header->stripe_padding = stripe_padding;

  return 0;
}

static
int get_stripe_padding(char *buf)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    PyErr_SetString(PyECLibError, "Invalid fragment header (stripe padding check)!");
    return -1;
  }

  return header->stripe_padding;
}

static
int validate_fragment(char *buf)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    return -1;
  }

  return 0;
}

static
int free_fragment_buffer(char *buf)
{
  fragment_header_t *header;
  if (buf == NULL) {
    return -1;
  }

  buf -= sizeof(fragment_header_t);

  header = (fragment_header_t*)buf;
  if (header->magic != PYECC_HEADER_MAGIC) {
    PyErr_SetString(PyECLibError, "Invalid fragment header (free fragment)!"); 
    return -1; 
  }
  free(buf);

  return 0;
}

static void pyeclib_c_destructor(PyObject *obj)
{
  pyeclib_t *pyeclib_handle = NULL;

  if (!PyCapsule_CheckExact(obj)) {
    PyErr_SetString(PyECLibError, "Attempted to free a non-Capsule object in pyeclib");
    return;
  }

  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(obj, PYECC_HANDLE_NAME);
  
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Attempted to free an invalid reference to pyeclib_handle");
    return;
  }
}

/*
 * Buffers for data, parity and missing_idxs
 * must be alloc'd by the caller.
 */
static int get_decoding_info(pyeclib_t *pyeclib_handle,
                             PyObject  *data_list, 
                             PyObject  *parity_list, 
                             PyObject  *missing_idx_list,
                             char      **data,
                             char      **parity,
                             int       *missing_idxs,
                             int       *stripe_padding,
                             int       fragment_size,
                             unsigned long long *realloc_bm)
{
  int i;
  int data_size, parity_size, missing_size;
  int padding = -1;
  unsigned long long missing_bm;
  int needs_addr_alignment = PYECLIB_NEEDS_ADDR_ALIGNMENT(pyeclib_handle->type);

  data_size = (int)PyList_Size(data_list);
  parity_size = (int)PyList_Size(parity_list);
  missing_size = (int)PyList_Size(missing_idx_list);
  
  /*
   * Validate the list sizes
   */
  if (data_size != pyeclib_handle->k) {
    return -1;
  }
  if (parity_size != pyeclib_handle->m) {
    return -1;
  }
  
  for (i = 0; i < missing_size; i++) {
    PyObject *obj_idx = PyList_GetItem(missing_idx_list, i);
    long idx = PyLong_AsLong(obj_idx);

    missing_idxs[i] = (int)idx;
  }
  missing_idxs[i] = -1; 

  missing_bm = convert_list_to_bitmap(missing_idxs);

  /*
   * Determine if each fragment is:
   * 1.) Alloc'd: if not, alloc new buffer (for missing fragments)
   * 2.) Aligned to 16-byte boundaries: if not, alloc a new buffer
   *     memcpy the contents and free the old buffer 
   */
  for (i=0; i < pyeclib_handle->k; i++) {
    PyObject *tmp_data = PyList_GetItem(data_list, i);
    Py_ssize_t len = 0;
    PyString_AsStringAndSize(tmp_data, &(data[i]), &len);
    
    /*
     * Replace with aligned buffer, if the buffer was not 
     * aligned.  DO NOT FREE: the python GC should free
     * the original when cleaning up 'data_list'
     */
    if (len == 0 || !data[i]) {
      // Missing item has not been allocated...  Allocate it
      data[i] = alloc_fragment_buffer(fragment_size-sizeof(fragment_header_t));
      *realloc_bm = *realloc_bm | (1 << i);
    } else if ((needs_addr_alignment && !is_addr_aligned((unsigned long)data[i], 16))) {
      char *tmp_buf = alloc_fragment_buffer(fragment_size-sizeof(fragment_header_t));
      memcpy(tmp_buf, data[i], fragment_size);
      data[i] = tmp_buf;
      *realloc_bm = *realloc_bm | (1 << i);
    }


    /*
     * Need to determine if the payload of the last data fragment
     * was padded.  we write the stripe padding to all fragments, so we
     * get it out of the first valid fragment.
     */
    if (((missing_bm & (1 << i)) == 0) && padding < 0) {
      padding = get_stripe_padding(data[i]);
      if (padding < 0) {
        return -1;
      }
    }

    data[i] = get_fragment_data(data[i]);
    if (data[i] == NULL) {
      return -1;
    }
  }

  for (i=0; i < pyeclib_handle->m; i++) {
    PyObject *tmp_parity = PyList_GetItem(parity_list, i);
    Py_ssize_t len = 0;
    PyString_AsStringAndSize(tmp_parity, &(parity[i]), &len);
    
    /*
     * Replace with aligned buffer, if the buffer was not 
     * aligned.  DO NOT FREE: the python GC should free
     * the original when cleaning up 'data_list'
     */
    if (len == 0 || !parity[i]) {
      // Missing item has not been allocated...  Allocate it
      parity[i] = alloc_fragment_buffer(fragment_size-sizeof(fragment_header_t));
      *realloc_bm = *realloc_bm | (1 << (pyeclib_handle->k + i));
    } else if (needs_addr_alignment && !is_addr_aligned((unsigned long)parity[i], 16)) {
      char *tmp_buf = alloc_fragment_buffer(fragment_size-sizeof(fragment_header_t));
      memcpy(tmp_buf, parity[i], fragment_size);
      parity[i] = tmp_buf;
      *realloc_bm = *realloc_bm | (1 << (pyeclib_handle->k + i));
    } 
    
    parity[i] = get_fragment_data(parity[i]);
    if (parity[i] == NULL) {
      return -1;
    }
  }

  *stripe_padding = padding;
  return 0; 
}

static PyObject *
pyeclib_c_init(PyObject *self, PyObject *args)
{
  pyeclib_t *pyeclib_handle;
  PyObject* pyeclib_obj_handle;
  int k, m, w;
  const char *type_str;
  pyeclib_type_t type;

  if (!PyArg_ParseTuple(args, "iiis", &k, &m, &w, &type_str)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.init");
    return NULL;
  }

  // Validate the arguments
  type = get_ecc_type(type_str);
  if (type == PYECC_NOT_FOUND) {
    PyErr_SetString(PyECLibError, "Invalid type passed to pyeclib.init");
    return NULL;
  }

  if (!validate_args(k, m, w, type)) {
    PyErr_SetString(PyECLibError, "Invalid args passed to pyeclib.init");
    return NULL;
  }
  
  pyeclib_handle = (pyeclib_t*)malloc(sizeof(pyeclib_t));
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "malloc() returned NULL in pyeclib.init");
    return NULL;
  }

  memset(pyeclib_handle, 0, sizeof(pyeclib_t));
  
  pyeclib_handle->k = k;
  pyeclib_handle->m = m;
  pyeclib_handle->w = w;
  pyeclib_handle->type = type;

  switch (type) {
    case PYECC_RS_CAUCHY_ORIG:
      pyeclib_handle->matrix = cauchy_original_coding_matrix(k, m, w);
      pyeclib_handle->bitmatrix = jerasure_matrix_to_bitmatrix(k, m, w, pyeclib_handle->matrix);
      pyeclib_handle->schedule = jerasure_smart_bitmatrix_to_schedule(k, m, w, pyeclib_handle->bitmatrix);
      break;
    case PYECC_XOR_HD_3:
      pyeclib_handle->xor_code_desc = init_xor_hd_code(k, m, 3);
      break;
    case PYECC_XOR_HD_4:
      pyeclib_handle->xor_code_desc = init_xor_hd_code(k, m, 4);
      break;
    case PYECC_RS_VAND:
    default:
      pyeclib_handle->matrix = reed_sol_vandermonde_coding_matrix(k, m, w);
      break;
  }

  pyeclib_obj_handle = PyCapsule_New(pyeclib_handle, PYECC_HANDLE_NAME, pyeclib_c_destructor);

  if (pyeclib_obj_handle == NULL) {
    PyErr_SetString(PyECLibError, "Could not encapsulate pyeclib_handle into Python object in pyeclib.init");
    return NULL;
  }

  Py_INCREF(pyeclib_obj_handle);

  return pyeclib_obj_handle;
}

static PyObject *
pyeclib_c_encode(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle;
  pyeclib_t *pyeclib_handle;
  char *data;
  int data_len;
  int blocksize, residual, stripe_padding = 0;
  char **data_to_encode;
  char **encoded_parity;
  int i;
  PyObject *list_of_strips;

  if (!PyArg_ParseTuple(args, "Os#", &pyeclib_obj_handle, &data, &data_len)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.encode");
    return NULL;
  }

  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.encode");
    return NULL;
  }

  if (pyeclib_handle->type == PYECC_RS_CAUCHY_ORIG) {
    int aligned_data_len = data_len;
    
    while (aligned_data_len % (pyeclib_handle->k*pyeclib_handle->w*PYECC_CAUCHY_PACKETSIZE) != 0) {
      aligned_data_len++;
    } 
    
    blocksize = aligned_data_len / pyeclib_handle->k;
    stripe_padding = aligned_data_len - data_len;

  } else {
    int num_words, fragment_padding;

    /*
     * Compute the blocksize 
     */
    blocksize = (int)ceill((double)data_len / pyeclib_handle->k);

    num_words = (int)ceill((double)blocksize / PYECLIB_WORD_SIZE(pyeclib_handle->type));
      
    fragment_padding = (num_words * PYECLIB_WORD_SIZE(pyeclib_handle->type)) - blocksize;

    blocksize += fragment_padding;

    /*
     * Since the buffer is probably not aligned, 
     * the last block will probably need stripe padding
     */
    residual = data_len % blocksize;
    if (residual > 0) {
      stripe_padding = blocksize - residual;
    }
  }

  data_to_encode = (char**)malloc(sizeof(char*)*pyeclib_handle->k);
  if (data_to_encode == NULL) {
    PyErr_SetString(PyECLibError, "Could not allocate memory in pyeclib.encode");
    return NULL;
  }

  for (i=0; i < pyeclib_handle->k; i++) {
    char *fragment = alloc_fragment_buffer(blocksize);
    data_to_encode[i] = get_fragment_data(fragment);
    if (data_to_encode[i] == NULL) {
      PyErr_SetString(PyECLibError, "Could not allocate memory in pyeclib.encode");
      return NULL;
    }
    memcpy(data_to_encode[i], data, data_len > blocksize ? blocksize : data_len);
    if (data_len > blocksize) {
      set_fragment_size(fragment, blocksize);
    } else {
      set_fragment_size(fragment, data_len);
    }
    data += (data_len > blocksize ? blocksize : data_len);
    data_len -= blocksize;
  }
  
  encoded_parity = (char**)malloc(sizeof(char*)*pyeclib_handle->m);
  if (encoded_parity == NULL) {
    PyErr_SetString(PyECLibError, "Could not allocate memory in pyeclib.encode");
    return NULL;
  }

  for (i=0; i < pyeclib_handle->m; i++) {
    char *fragment = alloc_fragment_buffer(blocksize);
    encoded_parity[i] = get_fragment_data(fragment);
    if (encoded_parity[i] == NULL) {
      PyErr_SetString(PyECLibError, "Could not allocate memory in pyeclib.encode");
      return NULL;
    }
    set_fragment_size(fragment, blocksize);
  }

  switch (pyeclib_handle->type) {
    case PYECC_RS_CAUCHY_ORIG:
      jerasure_bitmatrix_encode(pyeclib_handle->k, pyeclib_handle->m, pyeclib_handle->w, pyeclib_handle->bitmatrix, data_to_encode, encoded_parity, blocksize, PYECC_CAUCHY_PACKETSIZE);
      break;
    case PYECC_XOR_HD_3:
    case PYECC_XOR_HD_4:
      pyeclib_handle->xor_code_desc->encode(pyeclib_handle->xor_code_desc, data_to_encode, encoded_parity, blocksize);
      break;
    case PYECC_RS_VAND:
    default:
      jerasure_matrix_encode(pyeclib_handle->k, pyeclib_handle->m, pyeclib_handle->w, pyeclib_handle->matrix, data_to_encode, encoded_parity, blocksize);
      break;
  }

  list_of_strips = PyList_New(pyeclib_handle->k + pyeclib_handle->m);
  
  for (i=0; i < pyeclib_handle->k; i++) {
    char *fragment_ptr = get_fragment_ptr_from_data(data_to_encode[i]);
    int fragment_size = blocksize+sizeof(fragment_header_t);
    set_fragment_idx(fragment_ptr, i);
    set_stripe_padding(fragment_ptr, stripe_padding);
    PyList_SET_ITEM(list_of_strips, i, Py_BuildValue("s#", fragment_ptr, fragment_size));
    free_fragment_buffer(data_to_encode[i]);
  }
  free(data_to_encode);
  
  for (i=0; i < pyeclib_handle->m; i++) {
    char *fragment_ptr = get_fragment_ptr_from_data(encoded_parity[i]);
    int fragment_size = blocksize+sizeof(fragment_header_t);
    set_fragment_idx(fragment_ptr, pyeclib_handle->k+i);
    set_stripe_padding(fragment_ptr, stripe_padding);
    PyList_SET_ITEM(list_of_strips, pyeclib_handle->k + i, Py_BuildValue("s#", fragment_ptr, fragment_size));
    free_fragment_buffer(encoded_parity[i]);
  }
  free(encoded_parity);

  return list_of_strips;
}

/*
 * Convert a set of fragments into a string.  If, 
 * all k data elements are not present, return NULL;
 * otherwise return a string.
 */
static PyObject *
pyeclib_c_fragments_to_string(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle;
  PyObject *fragment_list;
  PyObject *ret_string;
  pyeclib_t* pyeclib_handle;
  char *ret_cstring;
  char **data;
  int string_len = 0;
  int string_off = 0;
  int i = 0;
  int num_fragments = 0;
  int num_data = 0;

  if (!PyArg_ParseTuple(args, "OO", &pyeclib_obj_handle, &fragment_list)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.fragments_to_string");
    return NULL;
  }
  
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.fragments_to_string");
    return NULL;
  }
  
  if (!PyList_Check(fragment_list)) {
    PyErr_SetString(PyECLibError, "Invalid structure passed in for fragment list in pyeclib.fragments_to_string");
    return NULL;
  }
  
  // Set the number of fragments we have
  num_fragments = (int)PyList_Size(fragment_list);

  /*
   * If we have less than k fragments, then we definitely need to call decode, so return NULL
   */
  if (pyeclib_handle->k > num_fragments) {
    return Py_BuildValue("");
  }
  
  data = (char**)malloc(sizeof(char*) * pyeclib_handle->k); 
  if (data == NULL) {
    PyErr_SetString(PyECLibError, "Could not allocate memory for data buffers");
    return NULL;
  }

  /*
   * Iterate over the fragments.  If we have all k data fragments, then we can
   * concatenate them into a string and return it; otherwise, we return NULL
   */
  for (i=0; i < num_fragments && num_data < pyeclib_handle->k; i++) {
    PyObject *tmp_data = PyList_GetItem(fragment_list, i);
    char *tmp_buf;
    int index;
    int data_size;
    Py_ssize_t len;

    PyString_AsStringAndSize(tmp_data, &tmp_buf, &len);
    
    /*
     * Get fragment index and size
     */
    index = get_fragment_idx(tmp_buf);
    if (index < 0) {
      ret_string = NULL;
      goto out;
    }
    data_size = get_fragment_size(tmp_buf);
    if (data_size < 0) {
      ret_string = NULL;
      goto out;
    }
    
    /*
     * Skip over parity fragments
     */
    if (index >= pyeclib_handle->k) {
      continue;
    }

    /*
     * Put fragment reference in proper index of data
     */
    data[index] = tmp_buf;

    // Increment the number of data fragments we have
    num_data++;
    string_len += data_size;
  }

  if (num_data != pyeclib_handle->k) {
    ret_string = Py_BuildValue("");
    goto out;
  }

  ret_cstring = (char*)malloc(string_len);

  /*
   * Copy data payloads into a cstring.  The
   * fragments should be ordered by index in data.
   */
  for (i=0; i < num_data; i++) {
    char* fragment_data = get_fragment_data(data[i]);
    int fragment_size = get_fragment_size(data[i]);

    memcpy(ret_cstring + string_off, fragment_data, fragment_size);
    string_off += fragment_size;
  }

  ret_string = Py_BuildValue("s#", ret_cstring, string_len);
  free(ret_cstring);

out:
  free(data);
  return ret_string;
}

/*
 * Return a tuple containing a refernece to a data fragment 
 * list, a parity fragment list and a missing fragment index list, 
 * including all '0' fragment for missing fragments.  Copy 
 * references to the existing  fragments and create new string 
 * for missing fragments.
 */
static PyObject *
pyeclib_c_get_fragment_partition(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle;
  PyObject *fragment_list;
  PyObject *data_list;
  PyObject *parity_list;
  PyObject *missing_list;
  PyObject *return_lists;
  PyObject **data;
  PyObject **parity;
  int  *missing;
  pyeclib_t* pyeclib_handle;
  int num_fragments;
  int num_missing = 0;
  int i;
  int fragment_size = 0;

  if (!PyArg_ParseTuple(args, "OO", &pyeclib_obj_handle, &fragment_list)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.get_fragment_partition");
    return NULL;
  }
  
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.get_fragment_partition");
    return NULL;
  }
  
  if (!PyList_Check(fragment_list)) {
    PyErr_SetString(PyECLibError, "Invalid structure passed in for fragment list in pyeclib.get_fragment_partition");
    return NULL;
  }
  
  // Set the number of fragments we have
  num_fragments = (int)PyList_Size(fragment_list);

  data = (PyObject**)malloc(pyeclib_handle->k * sizeof(PyObject*));
  parity = (PyObject**)malloc(pyeclib_handle->m * sizeof(PyObject*));

  for (i=0; i < pyeclib_handle->k; i++) {
    data[i] = NULL;
  }

  for (i=0; i < pyeclib_handle->m; i++) {
    parity[i] = NULL;
  }
  /*
  * ASSUMPTION: We will never try to do anything when we have more 
  * than k missing fragments
  */
  missing = (int*)malloc(pyeclib_handle->k * sizeof(int));
    
  /*
   * Fill in the data, parity and missing indexes
   */
  for (i=0; i < num_fragments; i++) {
    PyObject *tmp_data = PyList_GetItem(fragment_list, i);
    char *c_buf;
    int index;
    Py_ssize_t len;

    PyString_AsStringAndSize(tmp_data, &c_buf, &len);

    /*
     * Assume the fragment_size is the max of
     * the fragments we read
     */
    if (len > fragment_size) {
      fragment_size = (int)len;
    }
    
    /*
     * Get fragment index and size
     */
    index = get_fragment_idx(c_buf);

    if (index < pyeclib_handle->k) {
      // Data element
      data[index] = tmp_data;

    } else {
      // Parity element
      parity[index - pyeclib_handle->k] = tmp_data;
    }
  }

  /*
   * If there are missing bufs, figure out which indexes are missing
   */
  if (missing != NULL) {
    num_missing = 0;
    for (i=0; i < pyeclib_handle->k; i++) {
      if (data[i] == NULL) {
        missing[num_missing] = i;
        num_missing++;
      }
    }
    for (i=0; i < pyeclib_handle->m; i++) {
      if (parity[i] == NULL) {
        missing[num_missing] = i + pyeclib_handle->k;
        num_missing++;
      }
    }
  }

  
  data_list = PyList_New(pyeclib_handle->k);
  parity_list = PyList_New(pyeclib_handle->m);
  missing_list = PyList_New(num_missing);
  
  /*
   * Fill in the data fragments
   */
  for (i=0; i < pyeclib_handle->k; i++) {
    if (data[i] != NULL) {
      // This is a borrowed reference, so increment the ref count
      // before inserting into list
      Py_INCREF(data[i]);
      PyList_SET_ITEM(data_list, i, data[i]);
    } else {
      char *tmp_data = (char*)malloc(fragment_size);
      PyObject *zero_string;
      memset(tmp_data, 0, fragment_size);
      zero_string = Py_BuildValue("s#", tmp_data, fragment_size);
      //Py_INCREF(zero_string);
      PyList_SET_ITEM(data_list, i, zero_string);
      free(tmp_data);
    }
  }

  free(data);

  /*
   * Fill in the parity fragments
   */
  for (i=0; i < pyeclib_handle->m; i++) {
    if (parity[i] != NULL) {
      // This is a borrowed reference, so increment the ref count
      // before inserting into list
      Py_INCREF(parity[i]);
      PyList_SET_ITEM(parity_list, i, parity[i]);
    } else {
      char *tmp_parity = (char*)malloc(fragment_size);
      PyObject *zero_string;
      memset(tmp_parity, 0, fragment_size);
      zero_string = Py_BuildValue("s#", tmp_parity, fragment_size);
      //Py_INCREF(zero_string);
      PyList_SET_ITEM(parity_list, i, zero_string);
      free(tmp_parity);
    }
  }

  free(parity);

  for( i=0;i < num_missing; i++) {
    PyList_SET_ITEM(missing_list, i, Py_BuildValue("i", missing[i]));
  }

  free(missing);

  return_lists = PyTuple_New(3);
  
  Py_INCREF(data_list);
  Py_INCREF(parity_list);
  Py_INCREF(missing_list);

  PyTuple_SetItem(return_lists, 0, data_list);
  PyTuple_SetItem(return_lists, 1, parity_list);
  PyTuple_SetItem(return_lists, 2, missing_list);

  //Py_INCREF(return_lists);

  return return_lists;
}

static PyObject *
pyeclib_c_get_required_fragments(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle;
  PyObject *missing_list;
  PyObject *fragment_idx_list = NULL;
  pyeclib_t* pyeclib_handle;
  int num_missing;
  int *c_missing_list;
  int i, j;
  unsigned long missing_bm = 0;

  if (!PyArg_ParseTuple(args, "OO", &pyeclib_obj_handle, &missing_list)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.get_required_fragments");
    return NULL;
  }

  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.get_required_fragments");
    return NULL;
  }

  num_missing = (int)PyList_Size(missing_list);

  c_missing_list = (int*)malloc((num_missing+1)*sizeof(int));

  for (i=0; i < num_missing; i++) {
    PyObject *obj_idx = PyList_GetItem(missing_list, i);
    long idx = PyLong_AsLong(obj_idx);
    c_missing_list[i] = (int)idx;
  }

  c_missing_list[num_missing] = -1;

  missing_bm = convert_list_to_bitmap(c_missing_list);
      
  fragment_idx_list = PyList_New(0);

  switch(pyeclib_handle->type) {
    case PYECC_RS_CAUCHY_ORIG:
    case PYECC_RS_VAND:
      j=0;
      for (i=0; i < pyeclib_handle->k + pyeclib_handle->m; i++) {
        if (!(missing_bm & (1 << i))) {
          PyList_Append(fragment_idx_list, Py_BuildValue("i", i));
          j++;
        }
        if (j == pyeclib_handle->k) {
          break;
        }
      }

      if (j != pyeclib_handle->k) {
        // Let the garbage collector clean this up
        Py_DECREF(fragment_idx_list);
        PyErr_Format(PyECLibError, "Not enough fragments for pyeclib.get_required_fragments (need at least %d, %d are given)", pyeclib_handle->k, j);
        fragment_idx_list = NULL;
      }
      break;
    case PYECC_XOR_HD_3:
    case PYECC_XOR_HD_4:
    {
      int *fragments_needed = (int*)malloc(sizeof(int)*(pyeclib_handle->k+pyeclib_handle->m));
      int ret = pyeclib_handle->xor_code_desc->fragments_needed(pyeclib_handle->xor_code_desc, c_missing_list, fragments_needed);

      if (ret < 0) {
        Py_DECREF(fragment_idx_list);
        PyErr_Format(PyECLibError, "Not enough fragments for pyeclib.get_required_fragments!");
        fragment_idx_list = NULL;
        free(fragments_needed);
        break;
      }

      j=0;
      while (fragments_needed[j] > -1) {
        PyList_Append(fragment_idx_list, Py_BuildValue("i", fragments_needed[j]));
        j++;
      } 
      free(fragments_needed);
      break;
    }
    default:
      PyErr_SetString(PyECLibError, "Invalid EC type used in pyeclib.get_required_fragments");
      break;
  }

  // Free the temporary list
  free(c_missing_list);

  return fragment_idx_list;
}

/*
 * TODO: If we are reconstructing a parity element, ensure that all of the data elements are available!
 */
static PyObject *
pyeclib_c_reconstruct(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle;
  PyObject *data_list;
  PyObject *parity_list;
  PyObject *missing_idx_list;
  PyObject *reconstructed;
  int *erased;
  pyeclib_t *pyeclib_handle;
  int fragment_size;
  int blocksize;
  char **data;
  char **parity;
  int *missing_idxs;
  int destination_idx;
  unsigned long long realloc_bm = 0; // Identifies symbols that had to be allocated for alignment 
  int stripe_padding = -1;
  int missing_size;
  int *decoding_matrix;
  int *decoding_row;
  int *dm_ids;
  int ret;
  int i;

  if (!PyArg_ParseTuple(args, "OOOOii", &pyeclib_obj_handle, &data_list, &parity_list, &missing_idx_list, &destination_idx, &fragment_size)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.encode");
    return NULL;
  }
  
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.encode");
    return NULL;
  }

  if (!PyList_Check(data_list) || !PyList_Check(parity_list) || !PyList_Check(missing_idx_list)) {
    PyErr_SetString(PyECLibError, "Invalid structure passed in for data, parity and/or missing_idx list");
    return NULL;
  }

  if (pyeclib_handle->k != PyList_Size(data_list)) {
    PyErr_SetString(PyECLibError, "The data list does not have the correct number of entries");
    return NULL;
  }
  
  if (pyeclib_handle->m != PyList_Size(parity_list)) {
    PyErr_SetString(PyECLibError, "The parity list does not have the correct number of entries");
    return NULL;
  }

  blocksize = FRAGSIZE_2_BLOCKSIZE(fragment_size);

  missing_size = (int)PyList_Size(missing_idx_list);
  missing_idxs = (int*)malloc(sizeof(int)*(missing_size+1));
  if (missing_idxs == NULL) {
    PyErr_SetString(PyECLibError, "Could not allocate memory for missing indexes");
    return NULL;
  }

  data = (char**)malloc(sizeof(char*) * pyeclib_handle->k); 
  if (data == NULL) {
    PyErr_SetString(PyECLibError, "Could not allocate memory for data buffers");
    free(missing_idxs);
    return NULL;
  }
  
  parity = (char**)malloc(sizeof(char*) * pyeclib_handle->m); 
  if (parity == NULL) {
    PyErr_SetString(PyECLibError, "Could not allocate memory for parity buffers");
    free(missing_idxs);
    free(data);
    return NULL;
  }

  if (get_decoding_info(pyeclib_handle, data_list, parity_list, missing_idx_list, data, parity, missing_idxs, &stripe_padding, fragment_size, &realloc_bm)) {
    PyErr_SetString(PyECLibError, "Could not extract adequate decoding info from data, parity and missing lists");
    reconstructed = NULL;
    goto out; 
  }

  erased = jerasure_erasures_to_erased(pyeclib_handle->k, pyeclib_handle->m, missing_idxs);

  switch (pyeclib_handle->type) {
    case PYECC_RS_CAUCHY_ORIG:

      if (destination_idx < pyeclib_handle->k) {
        decoding_matrix = (int*)malloc(sizeof(int*)*pyeclib_handle->k*pyeclib_handle->k*pyeclib_handle->w*pyeclib_handle->w);
        dm_ids = (int*)malloc(sizeof(int)*pyeclib_handle->k);
        ret = jerasure_make_decoding_bitmatrix(pyeclib_handle->k, pyeclib_handle->m, pyeclib_handle->w, pyeclib_handle->bitmatrix, erased, decoding_matrix, dm_ids);
        decoding_row = decoding_matrix + (destination_idx*pyeclib_handle->k*pyeclib_handle->w*pyeclib_handle->w);    
      } else {
        dm_ids = NULL;
        decoding_row = pyeclib_handle->bitmatrix + ((destination_idx - pyeclib_handle->k)*pyeclib_handle->k*pyeclib_handle->w*pyeclib_handle->w);    
        ret = 0;
      }
      if (ret == 0) {
        jerasure_bitmatrix_dotprod(pyeclib_handle->k, pyeclib_handle->w, decoding_row, dm_ids, destination_idx, data, parity, blocksize, PYECC_CAUCHY_PACKETSIZE);
      }

      if (destination_idx < pyeclib_handle->k) {
        free(decoding_matrix);
      }
      free(dm_ids);

      break;
    case PYECC_RS_VAND:
  
      if (destination_idx < pyeclib_handle->k) {
        decoding_matrix = (int*)malloc(sizeof(int*)*pyeclib_handle->k*pyeclib_handle->k);
        dm_ids = (int*)malloc(sizeof(int)*pyeclib_handle->k);
        ret = jerasure_make_decoding_matrix(pyeclib_handle->k, pyeclib_handle->m, pyeclib_handle->w, pyeclib_handle->matrix, erased, decoding_matrix, dm_ids);
        decoding_row = decoding_matrix + (destination_idx * pyeclib_handle->k);
      } else {
        dm_ids = NULL;
        decoding_row = pyeclib_handle->matrix + ((destination_idx - pyeclib_handle->k) * pyeclib_handle->k);
        ret = 0;
      }
      if (ret == 0) {
        jerasure_matrix_dotprod(pyeclib_handle->k, pyeclib_handle->w, decoding_row, dm_ids, destination_idx, data, parity, blocksize);
      }

      if (destination_idx < pyeclib_handle->k) {
        free(decoding_matrix);
        free(dm_ids);
      }
  
      break;

    case PYECC_XOR_HD_3:
    case PYECC_XOR_HD_4:
      xor_reconstruct_one(pyeclib_handle->xor_code_desc, data, parity, missing_idxs, destination_idx, blocksize);
      ret = 0;
      break;

    default:
      ret = -1;
      break;
  }

  if (ret == 0) {
    char *fragment_ptr;
    if (destination_idx < pyeclib_handle->k) {
      fragment_ptr = get_fragment_ptr_from_data_novalidate(data[destination_idx]);
      init_fragment_header(fragment_ptr);
      set_fragment_idx(fragment_ptr, destination_idx);
      set_stripe_padding(fragment_ptr, stripe_padding);

      if (destination_idx < (pyeclib_handle->k-1)) {
        set_fragment_size(fragment_ptr, blocksize);
      } else {
        set_fragment_size(fragment_ptr, blocksize-stripe_padding);
      }
    } else {
      fragment_ptr = get_fragment_ptr_from_data_novalidate(parity[destination_idx - pyeclib_handle->k]);
      init_fragment_header(fragment_ptr);
      set_fragment_idx(fragment_ptr, destination_idx);
      set_stripe_padding(fragment_ptr, stripe_padding);
      set_fragment_size(fragment_ptr, blocksize);
    }

    reconstructed = Py_BuildValue("s#", fragment_ptr, fragment_size);

  } else {
    reconstructed = NULL;
  }
    
  for (i=0; i < pyeclib_handle->k; i++) {
    if (realloc_bm & (1 << i)) {
      char *ptr = get_fragment_ptr_from_data_novalidate(data[i]);
      free(ptr);
    }
  }
  for (i=0; i < pyeclib_handle->m; i++) {
    if (realloc_bm & (1 << (i + pyeclib_handle->k))) {
      char *ptr = get_fragment_ptr_from_data_novalidate(parity[i]);
      free(ptr);
    }
  }
  
out:

  free(missing_idxs);
  free(data);
  free(parity);

  return reconstructed;
}

static PyObject *
pyeclib_c_decode(PyObject *self, PyObject *args)
{
  PyObject *list_of_strips;
  PyObject *pyeclib_obj_handle;
  PyObject *data_list;
  PyObject *parity_list;
  PyObject *missing_idx_list;
  pyeclib_t *pyeclib_handle;
  int fragment_size;
  int blocksize;
  unsigned long long realloc_bm = 0; // Identifies symbols that had to be allocated for alignment 
  char **data;
  char **parity;
  int *missing_idxs;
  int missing_size;
  int stripe_padding = -1;
  int i, j;

  if (!PyArg_ParseTuple(args, "OOOOi", &pyeclib_obj_handle, &data_list, &parity_list, &missing_idx_list, &fragment_size)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.encode");
    return NULL;
  }

  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.encode");
    return NULL;
  }

  if (!PyList_Check(data_list) || !PyList_Check(parity_list) || !PyList_Check(missing_idx_list)) {
    PyErr_SetString(PyECLibError, "Invalid structure passed in for data, parity and/or missing_idx list");
    return NULL;
  }

  if (pyeclib_handle->k != PyList_Size(data_list)) {
    PyErr_SetString(PyECLibError, "The data list does not have the correct number of entries");
    return NULL;
  }
  
  if (pyeclib_handle->m != PyList_Size(parity_list)) {
    PyErr_SetString(PyECLibError, "The parity list does not have the correct number of entries");
    return NULL;
  }

  blocksize = FRAGSIZE_2_BLOCKSIZE(fragment_size);

  missing_size = (int)PyList_Size(missing_idx_list);
  missing_idxs = (int*)malloc(sizeof(int)*(missing_size+1));
  if (missing_idxs == NULL) {
    PyErr_SetString(PyECLibError, "Could not allocate memory for missing indexes");
    return NULL;
  }

  data = (char**)malloc(sizeof(char*) * pyeclib_handle->k);
  if (data == NULL) {
    PyErr_SetString(PyECLibError, "Could not allocate memory for data buffers");
    return NULL;
  }

  parity = (char**)malloc(sizeof(char*) * pyeclib_handle->m);
  if (parity == NULL) {
    PyErr_SetString(PyECLibError, "Could not allocate memory for parity buffers");
    return NULL;
  }

  if (get_decoding_info(pyeclib_handle, data_list, parity_list, missing_idx_list, data, parity, missing_idxs, &stripe_padding, fragment_size, &realloc_bm)) {
    PyErr_SetString(PyECLibError, "Could not extract adequate decoding info from data, parity and missing lists");
    return NULL;
  }

  switch (pyeclib_handle->type) {
    case PYECC_RS_CAUCHY_ORIG:
      jerasure_bitmatrix_decode(pyeclib_handle->k, pyeclib_handle->m, pyeclib_handle->w, pyeclib_handle->bitmatrix, 0, missing_idxs, data, parity, blocksize, PYECC_CAUCHY_PACKETSIZE);
      break;
    case PYECC_XOR_HD_3:
    case PYECC_XOR_HD_4:
      pyeclib_handle->xor_code_desc->decode(pyeclib_handle->xor_code_desc, data, parity, missing_idxs, blocksize, 1);
      break;
    case PYECC_RS_VAND:
    default:
      jerasure_matrix_decode(pyeclib_handle->k, pyeclib_handle->m, pyeclib_handle->w, pyeclib_handle->matrix, 1, missing_idxs, data, parity, blocksize);
      break;
  }

  list_of_strips = PyList_New(pyeclib_handle->k + pyeclib_handle->m);
  
  /*
   * Create headers for the newly decoded elements
   */
  j=0;
  while (missing_idxs[j] >= 0) {
    int missing_idx = missing_idxs[j];
    if (missing_idx < pyeclib_handle->k) {
      char *fragment_ptr = get_fragment_ptr_from_data_novalidate(data[missing_idx]);
      init_fragment_header(fragment_ptr);
      set_fragment_idx(fragment_ptr, missing_idx);
      set_stripe_padding(fragment_ptr, stripe_padding);

      /*
       * If we rebuilt the last data fragment, then we must
       * account for fragment size being less than fragment_size
       * due to stripe padding.
       */
      if (missing_idx < (pyeclib_handle->k-1)) {
        set_fragment_size(fragment_ptr, blocksize);
      } else {
        set_fragment_size(fragment_ptr, blocksize - stripe_padding);
      }
        
    } else if (missing_idx >= pyeclib_handle->k) {
      int parity_idx = missing_idx - pyeclib_handle->k;
      char *fragment_ptr = get_fragment_ptr_from_data_novalidate(parity[parity_idx]);
      init_fragment_header(fragment_ptr);
      set_fragment_idx(fragment_ptr, missing_idx);
      set_stripe_padding(fragment_ptr, stripe_padding);
      set_fragment_size(fragment_ptr, blocksize);
    } 
    j++;
  }

  /*
   * Fill in the data fragments
   */
  for (i=0; i < pyeclib_handle->k; i++) {
    char *fragment_ptr = get_fragment_ptr_from_data(data[i]);
    PyList_SET_ITEM(list_of_strips, i, Py_BuildValue("s#", fragment_ptr, fragment_size));
    if (realloc_bm & (1 << i)) {
      free(fragment_ptr);
    }
  }
  /*
   * Fill in the parity fragments
   */
  for (i=0; i < pyeclib_handle->m; i++) {
    char *fragment_ptr = get_fragment_ptr_from_data(parity[i]);
    PyList_SET_ITEM(list_of_strips, pyeclib_handle->k + i, Py_BuildValue("s#", fragment_ptr, fragment_size));
    if (realloc_bm & (1 << (i + pyeclib_handle->k))) {
      free(fragment_ptr);
    }
  }

  free(missing_idxs);
  free(data);
  free(parity);

  return list_of_strips;
}

static PyMethodDef PyECLibMethods[] = {
    {"init",  pyeclib_c_init, METH_VARARGS, "Initialize a new erasure encoder/decoder"},
    {"encode",  pyeclib_c_encode, METH_VARARGS, "Create parity using source data"},
    {"decode",  pyeclib_c_decode, METH_VARARGS, "Recover all lost data/parity"},
    {"reconstruct",  pyeclib_c_reconstruct, METH_VARARGS, "Recover selective data/parity"},
    {"fragments_to_string", pyeclib_c_fragments_to_string, METH_VARARGS, "Try to transform a set of fragments into original string without calling the decoder"},
    {"get_fragment_partition", pyeclib_c_get_fragment_partition, METH_VARARGS, "Parition fragments into data and parity, also returns a list of missing indexes"},
    {"get_required_fragments", pyeclib_c_get_required_fragments, METH_VARARGS, "Return the fragments required to reconstruct a set of missing fragments"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initpyeclib_c(void)
{
    PyObject *m;

    Py_Initialize();

    m = Py_InitModule("pyeclib_c", PyECLibMethods);
    if (m == NULL) {
      return;
    }

    PyECLibError = PyErr_NewException("pyeclib.Error", NULL, NULL);
    if (PyECLibError == NULL) {
      fprintf(stderr, "Could not create default PyECLib exception object!\n");
      exit(2);
    }
    Py_INCREF(PyECLibError);
    PyModule_AddObject(m, "error", PyECLibError);
}

