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

#include <Python.h>

/* Compat layer for python <= 2.6 */
#include "capsulethunk.h"

#include <xor_code.h>
#include <reed_sol.h>
#include <alg_sig.h>
#include <cauchy.h>
#include <jerasure.h>
#include <liberation.h>
#include <galois.h>
#include <math.h>
#include <pyeclib_c.h>
#include <bytesobject.h>

/* Python 3 compatibility macros */
#if PY_MAJOR_VERSION >= 3
  #define MOD_ERROR_VAL NULL
  #define MOD_SUCCESS_VAL(val) val
  #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
          static struct PyModuleDef moduledef = { \
              PyModuleDef_HEAD_INIT, name, doc, -1, methods, }; \
          ob = PyModule_Create(&moduledef);
  #define PY_BUILDVALUE_OBJ_LEN(obj, objlen) \
          Py_BuildValue("y#", obj, objlen)
  #define PyInt_FromLong PyLong_FromLong
  #define PyString_FromString PyUnicode_FromString
  #define ENCODE_ARGS "Oy#"
  #define GET_METADATA_ARGS "Oy#"
#else
  #define MOD_ERROR_VAL
  #define MOD_SUCCESS_VAL(val)
  #define MOD_INIT(name) void init##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
          ob = Py_InitModule3(name, methods, doc);
  #define PY_BUILDVALUE_OBJ_LEN(obj, objlen) \
          Py_BuildValue("s#", obj, objlen)
  #define ENCODE_ARGS "Os#"
  #define GET_METADATA_ARGS "Os#"
#endif


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


/**
 * Prototypes for Python/C API methods
 */
static PyObject * pyeclib_c_init(PyObject *self, PyObject *args);
static void pyeclib_c_destructor(PyObject *obj);
static PyObject * pyeclib_c_get_segment_info(PyObject *self, PyObject *args);
static PyObject * pyeclib_c_encode(PyObject *self, PyObject *args);
static PyObject * pyeclib_c_fragments_to_string(PyObject *self, PyObject *args);
static PyObject * pyeclib_c_get_fragment_partition(PyObject *self, PyObject *args);
static PyObject * pyeclib_c_reconstruct(PyObject *self, PyObject *args);
static PyObject * pyeclib_c_decode(PyObject *self, PyObject *args);
static PyObject * pyeclib_c_get_metadata(PyObject *self, PyObject *args);
static PyObject * pyeclib_c_check_metadata(PyObject *self, PyObject *args);


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
        long long max_symbols;
      
        if (w != 8 && w != 16 && w != 32) {
          return 0;
        }
        max_symbols = 1LL << w;
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


/**
 * Memory Management Methods
 * 
 * The following methods provide wrappers for allocating and deallocating
 * memory.  
 * 
 * Future discussions may want to consider moving closer to the recommended
 * guidelines in the Python\C API reference manual.  One potential issue,
 * however, may be how we enforce memory alignment in the Python heap.
 *
 * 2.7: https://docs.python.org/2.7/c-api/memory.html
 * 3.4: https://docs.python.org/3.4/c-api/memory.html
 */
 
/**
 * Allocate a zero-ed buffer of a specific size.  This method allocates from
 * the Python stack in order to comply with the recommendations of the
 * Python\C API.  On error, return NULL and call PyErr_NoMemory.
 *
 * @param size integer size in bytes of buffer to allocate
 * @return pointer to start of allocated buffer or NULL on error
 */
static
void * alloc_zeroed_buffer(int size)
{
  void * buf = NULL;  /* buffer to allocate and return */
  
  /* Allocate and zero the buffer, or set the appropriate error */
  buf = PyMem_Malloc((size_t) size);
  if (buf) {
    buf = memset(buf, 0, (size_t) size);
  } else {
    buf = (void *) PyErr_NoMemory();
  }

  return buf;
}


/**
 * Deallocate memory buffer if it's not NULL.  This methods returns NULL so 
 * that you can free and reset a buffer using a single line as follows:
 *
 * my_ptr = check_and_free_buffer(my_ptr);
 *
 * @return NULL
 */
static
void * check_and_free_buffer(void * buf)
{
  if (buf) PyMem_Free(buf);
  
  return NULL;
}


/**
 * Allocate and zero out a buffer aligned to a 16-byte boundary in order
 * to support 128-bit operations.  On error, call PyErr_NoMemory to set
 * the appropriate error string and return NULL.
 * 
 * @param size integer size in bytes of buffer to allocate
 * @return pointer to start of allocated buffer, or NULL on error
 */
static
void* alloc_aligned_buffer16(int size)
{
  void *buf = NULL;
  
  /*
   * Ensure all memory is aligned to
   * 16-byte boundaries to support 
   * 128-bit operations
   */
  if (posix_memalign(&buf, 16, size) < 0) {
    return PyErr_NoMemory();
  } else {
    memset(buf, 0, size);
  }
  
  return buf;
}


/**
 * Allocate an initialized fragment buffer.  On error, return NULL and 
 * preserve call to PyErr_NoMemory.  Note, all allocated memory is aligned 
 * to 16-bytes boundaries in order to support 128-bit operations.
 *
 * @param size integer size in bytes of buffer to allocate
 * @return pointer to start of allocated fragment or NULL on error
 */
static
char *alloc_fragment_buffer(int size)
{
  char *buf = NULL;
  fragment_header_t *header = NULL;

  /* Calculate size needed for fragment header + data */
  size += sizeof(fragment_header_t);

  /* Allocate and init the aligned buffer, or set the appropriate error */
  buf = alloc_aligned_buffer16(size);
  if (buf) {
    init_fragment_header(buf);
  }

  return buf;
}


/**
 * Deallocate a fragment buffer.  This method confirms via magic number that
 * the passed in buffer is a proper fragment buffer.
 *
 * @param buf pointer to fragment buffer
 * @return 0 on successful free, -1 on error
 */
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


/**
 * Allocate a zero-ed Python string of a specific size. On error, call
 * PyErr_NoMemory and return NULL.
 *
 * @param size integer size in bytes of zero string to create
 * @return pointer to start of allocated zero string or NULL on error
 */
static
PyObject * alloc_zero_string(int size)
{
  char *tmp_data = NULL;         /* tmp buffer used in PyObject creation */
  PyObject *zero_string = NULL;  /* zero string to return */
  
  /* Create the zero-out c-string buffer */
  tmp_data = (char *) alloc_zeroed_buffer(size);
  if (NULL == tmp_data) {
    return PyErr_NoMemory();
  }
  
  /* Create the python value to return */
  zero_string = PY_BUILDVALUE_OBJ_LEN(tmp_data, size);
  check_and_free_buffer(tmp_data);
  
  return zero_string;
}


static
char* get_data_ptr_from_fragment(char *buf)
{
  char * data_ptr = NULL;
  
  if (NULL != buf) {
    data_ptr = buf + sizeof(fragment_header_t);
  }
  
  return data_ptr;
}

static
char* get_fragment_ptr_from_data_novalidate(char *buf)
{
  buf -= sizeof(fragment_header_t);

  return buf;
}

static 
int use_inline_chksum(pyeclib_t *pyeclib_handle)
{
  return pyeclib_handle->inline_chksum;
}

static
int supports_alg_sig(pyeclib_t *pyeclib_handle)
{
  if (!pyeclib_handle->algsig_chksum) return 0;
  if (pyeclib_handle->alg_sig_desc != NULL) return 1;
  return 0;
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
int set_chksum(char *buf, int chksum)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    fprintf(stderr, "Invalid fragment header (set chksum)!\n");
    return -1; 
  }

  header->chksum = chksum;
  
  return 0;
}

static
int get_chksum(char *buf)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    PyErr_SetString(PyECLibError, "Invalid fragment header (get chksum)!");
    return -1;
  }

  return header->chksum;
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
}

static
int set_orig_data_size(char *buf, int orig_data_size)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    PyErr_SetString(PyECLibError, "Invalid fragment header (set orig data check)!");
    return -1; 
  }

  header->orig_data_size = orig_data_size;

  return 0;
}

static
int get_orig_data_size(char *buf)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    PyErr_SetString(PyECLibError, "Invalid fragment header (get orig data check)!");
    return -1;
  }

  return header->orig_data_size;
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


/*
 * Prepares the fragments and helper data structures for decoding.  Note, 
 * buffers for data, parity and missing_idxs must be allocated by the caller.
 *
 * @param *pyeclib_handle
 * @param *data_list python list of data fragments
 * @param *parity_list python list of parity fragments
 * @param *missing_idx_list python list of indexes of missing fragments
 * @param **data allocated k-length array of data buffers
 * @param **parity allocated m-length array of parity buffers
 * @param *missing_idxs array of missing indexes with extra space for -1 terminator
 * @param *orig_size original size of object from the fragment header
 * @param fragment_size integer size in bytes of fragment
 * @return 0 on success, -1 on error
 */
static int get_decoding_info(pyeclib_t *pyeclib_handle,
                             PyObject  *data_list, 
                             PyObject  *parity_list, 
                             PyObject  *missing_idx_list,
                             char      **data,
                             char      **parity,
                             int       *missing_idxs,
                             int       *orig_size,
                             int       fragment_size,
                             unsigned long long *realloc_bm)
{
  int i;                          /* a counter */
  int data_size = 0;              /* number of data fragments provided */
  int parity_size = 0;            /* number of parity fragments provided */
  int missing_size = 0;           /* number of missing index entries */
  int orig_data_size = -1;        /* data size (B) from fragment header */
  unsigned long long missing_bm;  /* bitmap form of missing indexes list */
  int needs_addr_alignment = PYECLIB_NEEDS_ADDR_ALIGNMENT(pyeclib_handle->type);
  
  /* Validate the list sizes */
  data_size = (int)PyList_Size(data_list);
  parity_size = (int)PyList_Size(parity_list);
  missing_size = (int)PyList_Size(missing_idx_list);  
  if (data_size != pyeclib_handle->k) {
    return -1;
  }
  if (parity_size != pyeclib_handle->m) {
    return -1;
  }
  
  /* Record missing indexes in a -1 terminated array and bitmap */
  for (i = 0; i < missing_size; i++) {
    PyObject *obj_idx = PyList_GetItem(missing_idx_list, i);
    
    missing_idxs[i] = (int) PyLong_AsLong(obj_idx);;
  }
  missing_idxs[i] = -1; 
  missing_bm = convert_list_to_bitmap(missing_idxs);

  /*
   * Determine if each data fragment is:
   * 1.) Alloc'd: if not, alloc new buffer (for missing fragments)
   * 2.) Aligned to 16-byte boundaries: if not, alloc a new buffer
   *     memcpy the contents and free the old buffer 
   */
  for (i = 0; i < pyeclib_handle->k; i++) {
    PyObject *tmp_data = PyList_GetItem(data_list, i);
    Py_ssize_t len = 0;
    PyBytes_AsStringAndSize(tmp_data, &(data[i]), &len);
    
    /*
     * Allocate or replace with aligned buffer if the buffer was not aligned.
     * DO NOT FREE: the python GC should free the original when cleaning up 'data_list'
     */
    if (len == 0 || !data[i]) {
      data[i] = alloc_fragment_buffer(fragment_size - sizeof(fragment_header_t));
      if (NULL == data[i]) {
        return -1;
      }
      *realloc_bm = *realloc_bm | (1 << i);
    } else if ((needs_addr_alignment && !is_addr_aligned((unsigned long)data[i], 16))) {
      char *tmp_buf = alloc_fragment_buffer(fragment_size - sizeof(fragment_header_t));
      memcpy(tmp_buf, data[i], fragment_size);
      data[i] = tmp_buf;
      *realloc_bm = *realloc_bm | (1 << i);
    }

    /* Need to determine the size of the original data */
    if (((missing_bm & (1 << i)) == 0) && orig_data_size < 0) {
      orig_data_size = get_orig_data_size(data[i]);
      if (orig_data_size < 0) {
        return -1;
      }
    }

    /* Set the data element to the fragment payload */
    data[i] = get_data_ptr_from_fragment(data[i]);
    if (data[i] == NULL) {
      return -1;
    }
  }

  /* Perform the same allocation, alignment checks on the parity fragments */
  for (i=0; i < pyeclib_handle->m; i++) {
    PyObject *tmp_parity = PyList_GetItem(parity_list, i);
    Py_ssize_t len = 0;
    PyBytes_AsStringAndSize(tmp_parity, &(parity[i]), &len);
    
    /*
     * Allocate or replace with aligned buffer, if the buffer was not aligned.
     * DO NOT FREE: the python GC should free the original when cleaning up 'data_list'
     */
    if (len == 0 || !parity[i]) {
      parity[i] = alloc_fragment_buffer(fragment_size-sizeof(fragment_header_t));
      if (NULL == parity[i]) {
        return -1;
      }
      *realloc_bm = *realloc_bm | (1 << (pyeclib_handle->k + i));
    } else if (needs_addr_alignment && !is_addr_aligned((unsigned long)parity[i], 16)) {
      char *tmp_buf = alloc_fragment_buffer(fragment_size-sizeof(fragment_header_t));
      memcpy(tmp_buf, parity[i], fragment_size);
      parity[i] = tmp_buf;
      *realloc_bm = *realloc_bm | (1 << (pyeclib_handle->k + i));
    } 
    
    /* Set the parity element to the fragment payload */
    parity[i] = get_data_ptr_from_fragment(parity[i]);
    if (parity[i] == NULL) {
      return -1;
    }
  }

  *orig_size = orig_data_size;
  return 0; 
}


/**
 * Compute a size aligned to the number of data and the underlying wordsize 
 * of the EC algorithm.
 * 
 * @param pyeclib_handle eclib object with EC configurations
 * @param data_len integer length of data in bytes
 * @return integer data length aligned with wordsize of EC algorithm
 */
static int
get_aligned_data_size(pyeclib_t* pyeclib_handle, int data_len)
{
  int word_size = pyeclib_handle->w / 8;
  int alignment_multiple;
  int aligned_size = 0;

  /*
   * For Cauchy reed-solomon align to k*word_size*packet_size
   * For Vandermonde reed-solomon and flat-XOR, align to k*word_size
   */
  if (pyeclib_handle->type == PYECC_RS_CAUCHY_ORIG) {
    alignment_multiple = pyeclib_handle->k * pyeclib_handle->w * PYECC_CAUCHY_PACKETSIZE;
  } else {
    alignment_multiple = pyeclib_handle->k * word_size;
  }

  aligned_size = (int)ceill((double)data_len / alignment_multiple) * alignment_multiple;

  return aligned_size;
}


static
int get_minimum_encode_size(pyeclib_t *pyeclib_handle)
{
  return get_aligned_data_size(pyeclib_handle, 1);
}


static
int get_fragment_metadata(pyeclib_t *pyeclib_handle, char *fragment_buf, fragment_metadata_t *fragment_metadata)
{
  char *fragment_data = get_data_ptr_from_fragment(fragment_buf);
  int fragment_size = get_fragment_size(fragment_buf);
  int fragment_idx = get_fragment_idx(fragment_buf);

  memset(fragment_metadata, 0, sizeof(fragment_metadata_t));

  fragment_metadata->size = fragment_size;
  fragment_metadata->idx = fragment_idx;

  /*
   * If w \in [8, 16] and using RS_VAND or XOR_CODE, then use Alg_sig
   * Else use CRC32
   */
  if (supports_alg_sig(pyeclib_handle)) {
    // Compute algebraic signature
    compute_alg_sig(pyeclib_handle->alg_sig_desc, fragment_data, fragment_size, fragment_metadata->signature);
  } else if (use_inline_chksum(pyeclib_handle)) {
    int stored_chksum = get_chksum(fragment_buf);
    int computed_chksum = crc32(0, fragment_data, fragment_size); 

    if (stored_chksum != computed_chksum) {
      fragment_metadata->chksum_mismatch = 1;
    }
  }

  return 0;
}


/**
 * Constructor method for creating a new pyeclib object using the given parameters.
 *
 * @param k integer number of data elements
 * @param m integer number of checksum elements
 * @param w integer word size in bytes
 * @param type_str string name of erasure coding algorithm
 * @return pointer to PyObject or NULL on error
 */
static PyObject *
pyeclib_c_init(PyObject *self, PyObject *args)
{
  pyeclib_t *pyeclib_handle = NULL;
  PyObject *pyeclib_obj_handle = NULL;
  int k, m, w;
  int use_inline_chksum = 0, use_algsig_chksum = 0;
  const char *type_str;
  pyeclib_type_t type;

  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, "iiis|ii", &k, &m, &w, &type_str, &use_inline_chksum, &use_algsig_chksum)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.init");
    return NULL;
  }
  type = get_ecc_type(type_str);
  if (type == PYECC_NOT_FOUND) {
    PyErr_SetString(PyECLibError, "Invalid type passed to pyeclib.init");
    return NULL;
  }
  if (!validate_args(k, m, w, type)) {
    PyErr_SetString(PyECLibError, "Invalid args passed to pyeclib.init");
    return NULL;
  }

  /* Allocate and initialize the pyeclib object */
  pyeclib_handle = (pyeclib_t *) alloc_zeroed_buffer(sizeof(pyeclib_t));
  if (!pyeclib_handle) {
    return NULL;
  }
  pyeclib_handle->k = k;
  pyeclib_handle->m = m;
  pyeclib_handle->w = w;
  pyeclib_handle->type = type;
  pyeclib_handle->inline_chksum = use_inline_chksum;
  pyeclib_handle->algsig_chksum = use_algsig_chksum;

  switch (type) {
    case PYECC_RS_CAUCHY_ORIG:
      pyeclib_handle->matrix = cauchy_original_coding_matrix(k, m, w);
      pyeclib_handle->bitmatrix = jerasure_matrix_to_bitmatrix(k, m, w, pyeclib_handle->matrix);
      pyeclib_handle->schedule = jerasure_smart_bitmatrix_to_schedule(k, m, w, pyeclib_handle->bitmatrix);
      break;
    case PYECC_XOR_HD_3:
      pyeclib_handle->xor_code_desc = init_xor_hd_code(k, m, 3);
      if (pyeclib_handle->algsig_chksum) {
        pyeclib_handle->alg_sig_desc = init_alg_sig(32, 16);
      }
      break;
    case PYECC_XOR_HD_4:
      pyeclib_handle->xor_code_desc = init_xor_hd_code(k, m, 4);
      if (pyeclib_handle->algsig_chksum) {
        pyeclib_handle->alg_sig_desc = init_alg_sig(32, 16);
      }
      break;
    case PYECC_RS_VAND:
    default:
      pyeclib_handle->matrix = reed_sol_vandermonde_coding_matrix(k, m, w);
      if (pyeclib_handle->algsig_chksum && w == 8) {
        pyeclib_handle->alg_sig_desc = init_alg_sig(32, 8);
      } else if (pyeclib_handle->algsig_chksum && w == 16) {
        pyeclib_handle->alg_sig_desc = init_alg_sig(32, 16);
      }
      break;
  }

  /* Prepare the python object to return */
#ifdef Py_CAPSULE_H
  pyeclib_obj_handle = PyCapsule_New(pyeclib_handle, PYECC_HANDLE_NAME,
                                     pyeclib_c_destructor);
#else
  pyeclib_obj_handle = PyCObject_FromVoidPtrAndDesc(pyeclib_handle,
                         (void *) PYECC_HANDLE_NAME,
             pyeclib_c_destructor);
#endif /* Py_CAPSULE_H */

  /* Clean up the allocated memory on error, or update the ref count */
  if (pyeclib_obj_handle == NULL) {
    PyErr_SetString(PyECLibError, "Could not encapsulate pyeclib_handle into Python object in pyeclib.init");
    check_and_free_buffer(pyeclib_handle);
  } else {
    Py_INCREF(pyeclib_obj_handle);
  }
  
  return pyeclib_obj_handle;
}


/**
 * Destructor method for cleaning up pyeclib object.
 */
static void 
pyeclib_c_destructor(PyObject *obj)
{
  pyeclib_t *pyeclib_handle = NULL;  /* pyeclib object to destroy */

  if (!PyCapsule_CheckExact(obj)) {
    PyErr_SetString(PyECLibError, "Attempted to free a non-Capsule object in pyeclib");
    return;
  }

  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(obj, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Attempted to free an invalid reference to pyeclib_handle");
  } else {
    check_and_free_buffer(pyeclib_handle);
  }
  
  return;
}


/**
 * This function takes data length and a segment size and returns an object 
 * containing:
 *
 * segment_size: size of the payload to give to encode()
 * last_segment_size: size of the payload to give to encode()
 * fragment_size: the fragment size returned by encode()
 * last_fragment_size: the fragment size returned by encode()
 * num_segments: number of segments 
 *
 * This allows the caller to prepare requests when segmenting a data stream 
 * to be EC'd.
 * 
 * Since the data length will rarely be aligned to the segment size, the last 
 * segment will be a different size than the others.  
 * 
 * There are restrictions on the length given to encode(), so calling this 
 * before encode is highly recommended when segmenting a data stream.
 *
 * Minimum segment size depends on the underlying EC type (if it is less 
 * than this, then the last segment will be slightly larger than the others, 
 * otherwise it will be smaller).
 *
 * @param pyeclib_obj_handle
 * @param data_len integer length of data in bytes
 * @param segment_size integer length of segment in bytes
 * @return a python dictionary with segment information
 * 
 */
static PyObject *
pyeclib_c_get_segment_info(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle = NULL;
  pyeclib_t *pyeclib_handle = NULL;
  PyObject *ret_dict = NULL;               /* python dictionary to return */
  int data_len;                            /* data length from user in bytes */
  int segment_size, last_segment_size;     /* segment sizes in bytes */
  int num_segments;                        /* total number of segments */
  int fragment_size, last_fragment_size;   /* fragment sizes in bytes */
  int min_segment_size;                    /* EC algorithm's min. size (B) */
  int aligned_segment_size;                /* size (B) adjusted for addr alignment */
  int aligned_data_len;                    /* size (B) adjusted for addr alignment */
  
  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, "Oii", &pyeclib_obj_handle, &data_len, &segment_size)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.encode");
    return NULL;
  }
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.encode");
    return NULL;
  }

  /* The minimum segment size depends on the EC algorithm */
  min_segment_size = get_minimum_encode_size(pyeclib_handle);

  /* Get the number of segments */
  num_segments = (int)ceill((double)data_len / segment_size);

  /*
   * If there are two segments and the last is smaller than the 
   * minimum size, then combine into a single segment
   */
  if (num_segments == 2 && data_len < (segment_size + min_segment_size)) {
    num_segments--;
  }

  /* Compute the fragment size from the segment size */
  if (num_segments == 1) {
    /*
     * There is one fragment, or two fragments, where the second is 
     * smaller than the min_segment_size, just create one segment
     */

    /*
     * This will copmpute a size algined to the number of data
     * and the underlying wordsize of the EC algorithm.
     */
    aligned_data_len = get_aligned_data_size(pyeclib_handle, data_len);

    /* aligned_data_len is guaranteed to be divisible by k */
    fragment_size = aligned_data_len / pyeclib_handle->k;

    /* Segment size is the user-provided segment size */
    segment_size = data_len;
    last_fragment_size = fragment_size;
    last_segment_size = segment_size;
  } else {
    /*
     * There will be at least 2 segments, where the last exceeeds
     * the minimum segment size.
     */

    aligned_segment_size = get_aligned_data_size(pyeclib_handle, segment_size);

    /* aligned_data_len is guaranteed to be divisible by k */
    fragment_size = aligned_segment_size / pyeclib_handle->k;

    last_segment_size = data_len - (segment_size * (num_segments - 1)); 

    /*
     * The last segment is lower than the minimum size, so combine it 
     * with the previous fragment
     */
    if (last_segment_size < min_segment_size) {
      // assert(num_segments > 2)?

      /* Add current "last segment" to second to last segment */
      num_segments--;
      last_segment_size = last_segment_size + segment_size;
    } 
    
    aligned_segment_size = get_aligned_data_size(pyeclib_handle, last_segment_size);
     
    /* Compute the last fragment size from the last segment size */
    last_fragment_size = aligned_segment_size / pyeclib_handle->k;
  }
  
  /* Add header to fragment sizes */
  last_fragment_size += sizeof(fragment_header_t);
  fragment_size += sizeof(fragment_header_t);

  /* Create and return the python dictionary of segment info */
  ret_dict = PyDict_New();
  if (NULL == ret_dict) {
    PyErr_SetString(PyECLibError, "Error allocating python dict in get_segment_info");
  } else {
    PyDict_SetItem(ret_dict, PyString_FromString("segment_size\0"), PyInt_FromLong(segment_size));
    PyDict_SetItem(ret_dict, PyString_FromString("last_segment_size\0"), PyInt_FromLong(last_segment_size));
    PyDict_SetItem(ret_dict, PyString_FromString("fragment_size\0"), PyInt_FromLong(fragment_size));
    PyDict_SetItem(ret_dict, PyString_FromString("last_fragment_size\0"), PyInt_FromLong(last_fragment_size));
    PyDict_SetItem(ret_dict, PyString_FromString("num_segments\0"), PyInt_FromLong(num_segments));
  }
  
  return ret_dict;
}


/**
 * Erasure encode a data buffer.
 *
 * @param pyeclib_obj_handle
 * @param data to encode
 * @return python list of encoded data and parity elements
 */
static PyObject *
pyeclib_c_encode(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle = NULL;
  pyeclib_t *pyeclib_handle= NULL; 
  char **data_to_encode = NULL;     /* array of k data buffers */
  char **encoded_parity = NULL;     /* array of m parity buffers */
  PyObject *list_of_strips = NULL;  /* list of encoded strips to return */
  char *data;                       /* param, data buffer to encode */
  int data_len;                     /* param, length of data buffer */
  int aligned_data_len;             /* EC algorithm compatible data length */
  int orig_data_size;               /* data length to write to headers */
  int blocksize;                    /* length of each of k data elements */
  int i;                            /* a counter */

  /* Assume binary data (force "byte array" input) */
  if (!PyArg_ParseTuple(args, ENCODE_ARGS, &pyeclib_obj_handle, &data, &data_len)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.encode");
    return NULL;
  }
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.encode");
    return NULL;
  }
  
  /* Calculate data sizes, aligned_data_len guaranteed to be divisible by k*/
  orig_data_size = data_len;
  aligned_data_len = get_aligned_data_size(pyeclib_handle, data_len);
  blocksize = aligned_data_len / pyeclib_handle->k;

  /* Allocate and initialize an array of zero'd out data buffers */
  data_to_encode = (char**) alloc_zeroed_buffer(sizeof(char*) * pyeclib_handle->k);
  if (NULL == data_to_encode) {
    goto error;
  }
  for (i = 0; i < pyeclib_handle->k; i++) {
    int payload_size = data_len > blocksize ? blocksize : data_len;
    char *fragment = alloc_fragment_buffer(blocksize);    
    if (NULL == fragment) {
      goto error;
    }
    
    /* Copy existing data into clean, zero'd out buffer */
    data_to_encode[i] = get_data_ptr_from_fragment(fragment);
    if (data_len > 0) {
      memcpy(data_to_encode[i], data, payload_size);
    }

    /* Fragment size will always be the same (may be able to get rid of this) */
    set_fragment_size(fragment, blocksize);

    data += payload_size;
    data_len -= payload_size;
  }

  /* Allocate and initialize an array of zero'd out parity buffers */
  encoded_parity = (char**) alloc_zeroed_buffer(sizeof(char*) * pyeclib_handle->m);
  if (NULL == encoded_parity) {
    goto error;
  }
  for (i = 0; i < pyeclib_handle->m; i++) {
    char *fragment = alloc_fragment_buffer(blocksize);
    if (NULL == fragment) {
      goto error;
    }
    
    encoded_parity[i] = get_data_ptr_from_fragment(fragment);
    set_fragment_size(fragment, blocksize);
  }

  /* Run the erasure coding algorithm to generate the parity fragments */
  switch (pyeclib_handle->type) {
    case PYECC_RS_CAUCHY_ORIG:
      jerasure_bitmatrix_encode(pyeclib_handle->k, pyeclib_handle->m, 
                                pyeclib_handle->w, pyeclib_handle->bitmatrix, 
                                data_to_encode, encoded_parity, blocksize, 
                                PYECC_CAUCHY_PACKETSIZE);
      break;
    case PYECC_XOR_HD_3:
    case PYECC_XOR_HD_4:
      pyeclib_handle->xor_code_desc->encode(pyeclib_handle->xor_code_desc, 
                                            data_to_encode, encoded_parity, 
                                            blocksize);
      break;
    case PYECC_RS_VAND:
    default:
      jerasure_matrix_encode(pyeclib_handle->k, pyeclib_handle->m, 
                             pyeclib_handle->w, pyeclib_handle->matrix, 
                             data_to_encode, encoded_parity, blocksize);
      break;
  }

  /* Create the python list of fragments to return */
  list_of_strips = PyList_New(pyeclib_handle->k + pyeclib_handle->m);
  if (NULL == list_of_strips) {
    PyErr_SetString(PyECLibError, "Error allocating python list in encode");
    goto error;
  }
  
  /* Finalize data fragments and add them to the python list to return */
  for (i = 0; i < pyeclib_handle->k; i++) {
    char *fragment_ptr = get_fragment_ptr_from_data(data_to_encode[i]);
    int fragment_size = blocksize + sizeof(fragment_header_t);
    set_fragment_idx(fragment_ptr, i);
    set_orig_data_size(fragment_ptr, orig_data_size);
    if (use_inline_chksum(pyeclib_handle)) {
      int chksum = crc32(0, data_to_encode[i], blocksize);
      set_chksum(fragment_ptr, chksum);
    }
    PyList_SET_ITEM(list_of_strips, i, 
                    PY_BUILDVALUE_OBJ_LEN(fragment_ptr, fragment_size));
  }
  
  /* Finalize parity fragments and add them to the python list to return */
  for (i = 0; i < pyeclib_handle->m; i++) {
    char *fragment_ptr = get_fragment_ptr_from_data(encoded_parity[i]);
    int fragment_size = blocksize + sizeof(fragment_header_t);
    set_fragment_idx(fragment_ptr, pyeclib_handle->k+i);
    set_orig_data_size(fragment_ptr, orig_data_size);
    if (use_inline_chksum(pyeclib_handle)) {
      int chksum = crc32(0, encoded_parity[i], blocksize);
      set_chksum(fragment_ptr, chksum);
    }
    PyList_SET_ITEM(list_of_strips, pyeclib_handle->k + i, 
                    PY_BUILDVALUE_OBJ_LEN(fragment_ptr, fragment_size));
  }
  
  goto exit;

error:
  list_of_strips = NULL;
  
exit:
  if (data_to_encode) {
    for (i = 0; i < pyeclib_handle->k; i++) {
      if (data_to_encode[i]) free_fragment_buffer(data_to_encode[i]);
    }
    check_and_free_buffer(data_to_encode);
  }
  if (encoded_parity) {
    for (i = 0; i < pyeclib_handle->m; i++) {
      if (encoded_parity[i]) free_fragment_buffer(encoded_parity[i]);
    }
    check_and_free_buffer(encoded_parity);
  }
  
  return list_of_strips;
}


/*
 * Convert a set of fragments into a string.  If, less than k data fragments 
 * are present, return None.  Return NULL on error.
 *
 * @param pyeclib_obj_handle
 * @param list of fragments
 * @param python string or None, NULL on error
 */
static PyObject *
pyeclib_c_fragments_to_string(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle = NULL;
  pyeclib_t *pyeclib_handle = NULL;
  PyObject *fragment_list = NULL;  /* param, python list of fragments */
  PyObject *ret_string = NULL;     /* python string to return */
  char *ret_cstring = NULL;        /* c string to build return string from */
  int ret_data_size;               /* size of return string in bytes */
  char **data = NULL;              /* array of data buffers */
  int string_off = 0;              /* offset into cstring */
  int num_fragments = 0;           /* num of fragments provided by caller */
  int num_data = 0;                /* num of fragments that are data */
  int orig_data_size = -1;         /* data size from fragment header */
  int i = 0;                       /* a counter */

  /* Collect and validate the method arguments */
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
  
  /* Return None if there's insufficient fragments */
  num_fragments = (int) PyList_Size(fragment_list);
  if (pyeclib_handle->k > num_fragments) {
    return Py_BuildValue("");
  }
    
  /*
   * NOTE: Update to only copy original size out of the buffers
   */

  /*
   * Iterate over the fragments.  If we have all k data fragments, then we can
   * concatenate them into a string and return it; otherwise, we return NULL
   */
  data = (char **) alloc_zeroed_buffer(sizeof(char *) * pyeclib_handle->k);
  if (NULL == data) {
    return NULL;
  }
  for (i = 0; i < num_fragments && num_data < pyeclib_handle->k; i++) {
    PyObject *tmp_data = PyList_GetItem(fragment_list, i);
    char *tmp_buf;
    int index;
    int data_size;
    Py_ssize_t len;

    /* Get and validate the fragment index and size */
    PyBytes_AsStringAndSize(tmp_data, &tmp_buf, &len);
    index = get_fragment_idx(tmp_buf);
    data_size = get_fragment_size(tmp_buf);
    if ((index < 0) || (data_size < 0)) {
      ret_string = NULL;
      goto exit;
    }

    /* Validate the original data size */
    if (orig_data_size < 0) {
      orig_data_size = get_orig_data_size(tmp_buf);
    } else {
      if (get_orig_data_size(tmp_buf) != orig_data_size) {
        PyErr_SetString(PyECLibError, "Inconsistent orig data sizes found in headers");
        ret_string = NULL;
        goto exit;
      }
    }
    
    /* Skip parity fragments, put data fragments in index order */
    if (index >= pyeclib_handle->k) {
      continue;
    } else {
      data[index] = tmp_buf;
      num_data++;
    }
  }

  /* Return None if there still isn't insufficient fragments */
  if (num_data != pyeclib_handle->k) {
    ret_string = Py_BuildValue("");
    goto exit;
  }
  
  /* Create the c-string to return */
  ret_cstring = (char *) alloc_zeroed_buffer(orig_data_size);
  if (NULL == ret_cstring) {
    ret_string = NULL;
    goto exit;
  }
  ret_data_size = orig_data_size;

  /* Copy fragment data into cstring (fragments should be in index order) */
  for (i = 0; i < num_data && orig_data_size > 0; i++) {
    char* fragment_data = get_data_ptr_from_fragment(data[i]);
    int fragment_size = get_fragment_size(data[i]);
    int payload_size = orig_data_size > fragment_size ? fragment_size : orig_data_size;

    memcpy(ret_cstring + string_off, fragment_data, payload_size);
    orig_data_size -= payload_size;
    string_off += payload_size;
  }
  ret_string = PY_BUILDVALUE_OBJ_LEN(ret_cstring, ret_data_size);

exit:
  check_and_free_buffer(data);
  check_and_free_buffer(ret_cstring);

  return ret_string;
}


/*
 * Return a tuple containing a reference to a data fragment list, a parity 
 * fragment list and a missing fragment index list, including all '0' 
 * fragment for missing fragments.  Copy references to the existing 
 * fragments and create new string for missing fragments.
 *
 * @param pyeclib_obj_handle
 * @param list of fragments
 * @return (data fragments, parity fragments, missing indexes) tuple
 */
static PyObject *
pyeclib_c_get_fragment_partition(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle = NULL;
  pyeclib_t* pyeclib_handle = NULL;
  PyObject *fragment_list = NULL;     /* param, list of fragments */
  PyObject *data_list = NULL;         /* data fragments for return tuple */
  PyObject *parity_list = NULL;       /* parity fragments for return tuple */
  PyObject *fragment_string = NULL;   /* fragment string for lists */
  PyObject *missing_list = NULL;      /* missing indexes for return tuple */
  PyObject *return_lists = NULL;      /* Python tuple to return */
  PyObject **data = NULL;             /* array of k data fragment buffers*/
  PyObject **parity = NULL;           /* array of m parity fragment buffers */
  int *missing = NULL;                /* indicies of missing fragments */
  int num_missing = 0;                /* num of missing fragments */
  int num_fragments;                  /* num of frags provided by caller */
  int fragment_size = 0;              /* size in bytes of fragments */
  int i = 0;                          /* a counter */

  /* Collect and validate the method arguments */
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
  
  /* Create the arrays need to hold the data and parity fragments */
  data = (PyObject **) alloc_zeroed_buffer(pyeclib_handle->k * sizeof(PyObject*));
  if (NULL == data) {
    goto error;
  }
  parity = (PyObject **) alloc_zeroed_buffer(pyeclib_handle->m * sizeof(PyObject*));
  if (NULL == parity) {
    goto error;
  }
      
  /* Fill in the data and parity pointers to reveal missing fragments */
  num_fragments = (int) PyList_Size(fragment_list);
  for (i = 0; i < num_fragments; i++) {
    PyObject *tmp_data = PyList_GetItem(fragment_list, i);
    char *c_buf;
    int index;
    Py_ssize_t len;
    
    /* ASSUMPTION: The fragment_size is the max of the fragments we read */
    PyBytes_AsStringAndSize(tmp_data, &c_buf, &len);
    if (len > fragment_size) {
      fragment_size = (int) len;
    }
    
    /* Get fragment index and set the index in data or parity */
    index = get_fragment_idx(c_buf);
    if (index < pyeclib_handle->k) {
      data[index] = tmp_data;
    } else {
      parity[index - pyeclib_handle->k] = tmp_data;
    }
  }
  
  /* If there are missing bufs, figure out which indexes are missing
   *
   * ASSUMPTION: We will never try to do anything when we have more 
   * than k missing fragments
   */
  missing = (int *) alloc_zeroed_buffer(pyeclib_handle->k * sizeof(int));
  if (NULL == missing) {
    goto error;
  } else {
    num_missing = 0;
    for (i = 0; i < pyeclib_handle->k; i++) {
      if (data[i] == NULL) {
        missing[num_missing] = i;
        num_missing++;
      }
    }
    for (i = 0; i < pyeclib_handle->m; i++) {
      if (parity[i] == NULL) {
        missing[num_missing] = i + pyeclib_handle->k;
        num_missing++;
      }
    }
  }
  
  /* Create the python objects to return */
  data_list = PyList_New(pyeclib_handle->k);
  parity_list = PyList_New(pyeclib_handle->m);
  missing_list = PyList_New(num_missing);
  return_lists = PyTuple_New(3);
  if (!data_list || !parity_list || !missing_list || !return_lists) {
    return_lists = PyErr_NoMemory();
    goto exit;
  }
  
  /* Fill in the data fragments, create zero fragment if missing */
  for (i = 0; i < pyeclib_handle->k; i++) {
    if (data[i] != NULL) {
      /* BORROWED REF: increment ref count before inserting into list */
      Py_INCREF(data[i]);
      fragment_string = data[i];
    } else {
      fragment_string = alloc_zero_string(fragment_size);
      if (NULL == fragment_string) {
        goto error;
      }
    }
    PyList_SET_ITEM(data_list, i, fragment_string);
  }

  /* Fill in the parity fragments, create zero fragment if missing */
  for (i = 0; i < pyeclib_handle->m; i++) {
    if (parity[i] != NULL) {
      /* BORROWED REF: increment ref count before inserting into list */
      Py_INCREF(parity[i]);
      fragment_string = parity[i];
    } else {
      fragment_string = alloc_zero_string(fragment_size);
      if (NULL == fragment_string) {
        goto error;
      }
    }
    PyList_SET_ITEM(parity_list, i, fragment_string);
  }

  /* Fill in the list of missing indexes */
  for (i = 0;i < num_missing; i++) {
    PyList_SET_ITEM(missing_list, i, Py_BuildValue("i", missing[i]));
  }
  
  Py_INCREF(data_list);
  Py_INCREF(parity_list);
  Py_INCREF(missing_list);
  
  PyTuple_SetItem(return_lists, 0, data_list);
  PyTuple_SetItem(return_lists, 1, parity_list);
  PyTuple_SetItem(return_lists, 2, missing_list);
  //Py_INCREF(return_lists);

  goto exit;

error:
  return_lists = NULL;

exit:
  check_and_free_buffer(data);
  check_and_free_buffer(parity);
  check_and_free_buffer(missing);
  
  return return_lists;
}


/**
 * Return a list of lists with valid rebuild indexes given an EC algorithm
 * and a list of missing indexes.
 *
 * @param pyeclib_obj_handle
 * @param missing_list indexes of missing fragments
 * @return a list of lists of indexes to rebuild data from
 */
static PyObject *
pyeclib_c_get_required_fragments(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle = NULL;
  pyeclib_t *pyeclib_handle = NULL;
  PyObject *missing_list = NULL;        /* param, */
  PyObject *fragment_idx_list = NULL;   /* list of req'd indexes to return */
  int *c_missing_list = NULL;           /* c-array of missing indexes */
  int num_missing;                      /* size of passed in missing list */
  int i = 0, j = 0;                     /* counters */
  int k, m;                             /* EC algorithm parameters */
  unsigned long missing_bm = 0;         /* bitmap of missing indexes */
  int *fragments_needed = NULL;         /* indexes of xor code fragments */
  int ret;                              /* return value for xor code */

  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, "OO", &pyeclib_obj_handle, &missing_list)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.get_required_fragments");
    return NULL;
  }
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.get_required_fragments");
    return NULL;
  }
  k = pyeclib_handle->k;
  m = pyeclib_handle->m;

  /* Generate -1 terminated c-array and bitmap of missing indexes */
  num_missing = (int) PyList_Size(missing_list);
  c_missing_list = (int *) alloc_zeroed_buffer((num_missing + 1) * sizeof(int));
  if (NULL == c_missing_list) {
    return NULL;
  }
  c_missing_list[num_missing] = -1;
  for (i = 0; i < num_missing; i++) {
    PyObject *obj_idx = PyList_GetItem(missing_list, i);
    long idx = PyLong_AsLong(obj_idx);
    c_missing_list[i] = (int) idx;
  }
  missing_bm = convert_list_to_bitmap(c_missing_list);
   
  /* Generate the python list of lists of rebuild indexes to return */   
  fragment_idx_list = PyList_New(0);
  if (NULL == fragment_idx_list) {
    goto exit;
  }
  j = 0;
  switch(pyeclib_handle->type) {
    case PYECC_RS_CAUCHY_ORIG:
    case PYECC_RS_VAND:
      for (i = 0; i < (k + m); i++) {
        if (!(missing_bm & (1 << i))) {
          PyList_Append(fragment_idx_list, Py_BuildValue("i", i));
          j++;
        }
        if (j == k) {
          break;
        }
      }

      if (j != k) {
        /* Let the garbage collector clean this up */
        Py_DECREF(fragment_idx_list);
        PyErr_Format(PyECLibError, "Not enough fragments for pyeclib.get_required_fragments (need at least %d, %d are given)", k, j);
        fragment_idx_list = NULL;
      }
      break;
    case PYECC_XOR_HD_3:
    case PYECC_XOR_HD_4:
    {
      fragments_needed = alloc_zeroed_buffer(sizeof(int) * (k + m));
      if (NULL == fragments_needed) {
        fragment_idx_list = NULL;
        goto exit;
      }
      ret = pyeclib_handle->xor_code_desc->fragments_needed(pyeclib_handle->xor_code_desc,
                                                            c_missing_list, 
                                                            fragments_needed);

      if (ret < 0) {
        Py_DECREF(fragment_idx_list);
        PyErr_Format(PyECLibError, "Not enough fragments for pyeclib.get_required_fragments!");
        fragment_idx_list = NULL;
        break;
      }

      while (fragments_needed[j] > -1) {
        PyList_Append(fragment_idx_list, Py_BuildValue("i", fragments_needed[j]));
        j++;
      }
      break;
    }
    default:
      PyErr_SetString(PyECLibError, "Invalid EC type used in pyeclib.get_required_fragments");
      break;
  }

exit:
  check_and_free_buffer(c_missing_list);
  check_and_free_buffer(fragments_needed);

  return fragment_idx_list;
}


/**
 * Reconstruct a missing fragment from the the remaining fragments.
 *
 * TODO: If we are reconstructing a parity element, ensure that all of the 
 * data elements are available!
 *
 * @param pyeclib_obj_handle
 * @param data_list k length list of data elements
 * @param parity_list m length list of parity elements
 * @param missing_idx_list list of the indexes of missing elements
 * @param destination_idx index of fragment to reconstruct
 * @param fragment_size size in bytes of the fragments
 * @return reconstructed destination fragment or NULL on error
 */
static PyObject *
pyeclib_c_reconstruct(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle = NULL;
  pyeclib_t *pyeclib_handle = NULL;
  PyObject *data_list = NULL;           /* param, list of data fragments */
  PyObject *parity_list = NULL;         /* param, list of parity fragments */
  PyObject *missing_idx_list = NULL;    /* param, list of missing indexes */
  PyObject *reconstructed = NULL;       /* reconstructed object to return */
  int *erased = NULL;                   /* jerasure notation of erased devs */
  int fragment_size;                    /* param, size in bytes of fragment */
  int blocksize;                        /* size in bytes, fragment - header */
  char **data = NULL;                   /* k length array of data buffers */
  char **parity = NULL;                 /* m length array of parity buffers */
  int *missing_idxs = NULL;             /* array of missing indexes */
  int destination_idx;                  /* param, index to reconstruct */
  unsigned long long realloc_bm = 0;    /* bitmap, which fragments were realloc'ed */
  int orig_data_size = -1;              /* data size (B),from fragment hdr */
  int missing_size;                     /* number of missing indexes */
  int *decoding_matrix = NULL;          /* reconstruct specific decode matrix */
  int *decoding_row = NULL;             /* required row from decode matrix */
  int *dm_ids = NULL;                   /* k length array of surviving indexes */
  int k, m, w;                          /* EC algorithm parameters */
  int ret;                              /* decode matrix creation return val */
  int i = 0;                            /* a counter */

  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, "OOOOii", &pyeclib_obj_handle, &data_list, 
                        &parity_list, &missing_idx_list, &destination_idx, 
                        &fragment_size)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.encode");
    return NULL;
  }
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.encode");
    return NULL;
  }
  k = pyeclib_handle->k;
  m = pyeclib_handle->m;
  w = pyeclib_handle->w;
  if (!PyList_Check(data_list) || !PyList_Check(parity_list) || !PyList_Check(missing_idx_list)) {
    PyErr_SetString(PyECLibError, "Invalid structure passed in for data, parity and/or missing_idx list");
    return NULL;
  }
  if (k != PyList_Size(data_list)) {
    PyErr_SetString(PyECLibError, "The data list does not have the correct number of entries");
    return NULL;
  }
  if (m != PyList_Size(parity_list)) {
    PyErr_SetString(PyECLibError, "The parity list does not have the correct number of entries");
    return NULL;
  }

  /* Allocate data structures needed for reconstruction */
  blocksize = FRAGSIZE_2_BLOCKSIZE(fragment_size);
  missing_size = (int) PyList_Size(missing_idx_list);
  missing_idxs = (int *) alloc_zeroed_buffer(sizeof(int) * (missing_size + 1));
  data = (char **) alloc_zeroed_buffer(sizeof(char *) * k);
  parity = (char **) alloc_zeroed_buffer(sizeof(char *) * m);
  if (NULL == missing_idxs || NULL == data || NULL == parity) {
    goto error;
  }
  
  /* Prepare for decoding, no need to go further on error */
  if (get_decoding_info(pyeclib_handle, data_list, parity_list, 
                        missing_idx_list, data, parity, missing_idxs, 
                        &orig_data_size, fragment_size, &realloc_bm)) {
    PyErr_SetString(PyECLibError, "Could not extract adequate decoding info from data, parity and missing lists");
    goto error;
  }

  /* Create the decoding matrix, and attempt reconstruction */
  erased = jerasure_erasures_to_erased(k, m, missing_idxs);
  switch (pyeclib_handle->type) {
    case PYECC_RS_CAUCHY_ORIG:
      if (destination_idx < k) {
        decoding_matrix = (int *) alloc_zeroed_buffer(sizeof(int*) * k * k * w * w);
        dm_ids = (int *) alloc_zeroed_buffer(sizeof(int) * k);
        if (NULL == decoding_matrix || NULL == dm_ids) {
          goto error;
        }
        ret = jerasure_make_decoding_bitmatrix(k, m, w, pyeclib_handle->bitmatrix, 
                                               erased, decoding_matrix, dm_ids);
        decoding_row = decoding_matrix + (destination_idx * k * w * w);
      } else {
        ret = 0;
        decoding_row = pyeclib_handle->bitmatrix + ((destination_idx - k) * k * w * w);    
      }
      
      if (ret == 0) {
        jerasure_bitmatrix_dotprod(k, w, decoding_row, dm_ids, destination_idx, 
                                   data, parity, blocksize, 
                                   PYECC_CAUCHY_PACKETSIZE);
      }
      break;
    case PYECC_RS_VAND:
      if (destination_idx < k) {
        decoding_matrix = (int *) alloc_zeroed_buffer(sizeof(int*) * k * k);
        dm_ids = (int *) alloc_zeroed_buffer(sizeof(int) * k);
        if (NULL == decoding_matrix || NULL == dm_ids) {
          goto error;
        }
        ret = jerasure_make_decoding_matrix(k, m, w, pyeclib_handle->matrix, 
                                            erased, decoding_matrix, dm_ids);
        decoding_row = decoding_matrix + (destination_idx * k);
      } else {
        ret = 0;
        decoding_row = pyeclib_handle->matrix + ((destination_idx - k) * k);
      }
      
      if (ret == 0) {
        jerasure_matrix_dotprod(k, w, decoding_row, dm_ids, destination_idx, 
                                data, parity, blocksize);
      }
      break;
    case PYECC_XOR_HD_3:
    case PYECC_XOR_HD_4:
      ret = 0;
      xor_reconstruct_one(pyeclib_handle->xor_code_desc, data, parity, 
                          missing_idxs, destination_idx, blocksize);
      break;
    default:
      ret = -1;
      break;
  }

  /* Set the metadata on the reconstructed fragment */
  if (ret == 0) {
    char *fragment_ptr = NULL;
    if (destination_idx < k) {
      fragment_ptr = get_fragment_ptr_from_data_novalidate(data[destination_idx]);
      init_fragment_header(fragment_ptr);
      set_fragment_idx(fragment_ptr, destination_idx);
      set_orig_data_size(fragment_ptr, orig_data_size);
      set_fragment_size(fragment_ptr, blocksize);
      if (use_inline_chksum(pyeclib_handle)) {
        int chksum = crc32(0, data[destination_idx], blocksize);
        set_chksum(fragment_ptr, chksum);
      }
    } else {
      fragment_ptr = get_fragment_ptr_from_data_novalidate(parity[destination_idx - k]);
      init_fragment_header(fragment_ptr);
      set_fragment_idx(fragment_ptr, destination_idx);
      set_orig_data_size(fragment_ptr, orig_data_size);
      set_fragment_size(fragment_ptr, blocksize);
      if (use_inline_chksum(pyeclib_handle)) {
        int chksum = crc32(0, parity[destination_idx - k], blocksize);
        set_chksum(fragment_ptr, chksum);
      }
    }

    reconstructed = PY_BUILDVALUE_OBJ_LEN(fragment_ptr, fragment_size);
  } else {
    reconstructed = NULL;
  }
  
  goto out;

error:
  reconstructed = NULL;
  
out:
  /* Free fragment buffers that needed to be reallocated for alignment */
  for (i = 0; i < k; i++) {
    if (realloc_bm & (1 << i)) {
      free(get_fragment_ptr_from_data_novalidate(data[i]));
    }
  }
  for (i = 0; i < m; i++) {
    if (realloc_bm & (1 << (i + k))) {
      free(get_fragment_ptr_from_data_novalidate(parity[i]));
    }
  }

  check_and_free_buffer(missing_idxs);
  check_and_free_buffer(data);
  check_and_free_buffer(parity);
  check_and_free_buffer(decoding_matrix);
  check_and_free_buffer(dm_ids);

  return reconstructed;
}


/**
 * Reconstruct all of the missing fragments from a set of fragments.
 *
 * TODO: There's a lot of duplicated code between this and the 
 * reconstruct method.  Consider refactoring these methods.
 *
 * @param pyeclib_obj_handle
 * @param data_list k length list of data elements
 * @param parity_list m length list of parity elements
 * @param missing_idx_list list of the indexes of missing elements
 * @param fragment_size size in bytes of the fragments
 * @return list of fragments
 */
static PyObject *
pyeclib_c_decode(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle = NULL;
  pyeclib_t *pyeclib_handle = NULL;
  PyObject *list_of_strips = NULL;    /* list of strips to return */
  PyObject *data_list = NULL;         /* param, list of data fragments */
  PyObject *parity_list = NULL;       /* param, list of data fragments */
  PyObject *missing_idx_list = NULL;  /* param, list of missing indexes */
  int fragment_size;                  /* param, size in bytes of fragment */
  int blocksize;                      /* size in bytes, fragment - header */
  unsigned long long realloc_bm = 0;  /* bitmap, which fragments were realloc'ed */
  char **data = NULL;                 /* k length array of data buffers */
  char **parity = NULL;               /* m length array of parity buffers */
  int *missing_idxs = NULL;           /* array of missing indexes */
  int missing_size;                   /* number of missing indexes */
  int orig_data_size = -1;            /* data size in bytes ,from fragment hdr */
  int k, m, w;                        /* EC algorithm parameters */
  int i = 0, j = 0;                   /* counters */

  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, "OOOOi", &pyeclib_obj_handle, &data_list, &parity_list, &missing_idx_list, &fragment_size)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.encode");
    return NULL;
  }
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.encode");
    return NULL;
  }
  k = pyeclib_handle->k;
  m = pyeclib_handle->m;
  w = pyeclib_handle->w;
  if (!PyList_Check(data_list) || !PyList_Check(parity_list) || !PyList_Check(missing_idx_list)) {
    PyErr_SetString(PyECLibError, "Invalid structure passed in for data, parity and/or missing_idx list");
    return NULL;
  }
  if (k != PyList_Size(data_list)) {
    PyErr_SetString(PyECLibError, "The data list does not have the correct number of entries");
    return NULL;
  }
  if (m != PyList_Size(parity_list)) {
    PyErr_SetString(PyECLibError, "The parity list does not have the correct number of entries");
    return NULL;
  }

  /* Allocate data structures needed for reconstruction */
  blocksize = FRAGSIZE_2_BLOCKSIZE(fragment_size);
  missing_size = (int) PyList_Size(missing_idx_list);
  missing_idxs = (int *) alloc_zeroed_buffer(sizeof(int) * (missing_size + 1));
  data = (char **) alloc_zeroed_buffer(sizeof(char *) * k);
  parity = (char **) alloc_zeroed_buffer(sizeof(char *) * m);
  if (NULL == missing_idxs || NULL == data || NULL == parity) {
    goto error;
  }

  /* Prepare for decoding, no need to go further on error */
  if (get_decoding_info(pyeclib_handle, data_list, parity_list, missing_idx_list, 
                        data, parity, missing_idxs, &orig_data_size, 
                        fragment_size, &realloc_bm)) {
    PyErr_SetString(PyECLibError, "Could not extract adequate decoding info from data, parity and missing lists");
    return NULL;
  }

  /* Reconstruct the missing fragments */
  switch (pyeclib_handle->type) {
    case PYECC_RS_CAUCHY_ORIG:
      jerasure_bitmatrix_decode(k, m, w, pyeclib_handle->bitmatrix, 0, missing_idxs, 
                                data, parity, blocksize, PYECC_CAUCHY_PACKETSIZE);
      break;
    case PYECC_XOR_HD_3:
    case PYECC_XOR_HD_4:
      pyeclib_handle->xor_code_desc->decode(pyeclib_handle->xor_code_desc, data, 
                                            parity, missing_idxs, blocksize, 1);
      break;
    case PYECC_RS_VAND:
    default:
      jerasure_matrix_decode(k, m, w, pyeclib_handle->matrix, 1, missing_idxs, 
                            data, parity, blocksize);
      break;
  }

  /* Create the python list to return */
  list_of_strips = PyList_New(k + m);
  if (NULL == list_of_strips) {
    goto error;
  }
  
  /* Create headers for the newly decoded elements */
  while (missing_idxs[j] >= 0) {
    int missing_idx = missing_idxs[j];
    if (missing_idx < k) {
      char *fragment_ptr = get_fragment_ptr_from_data_novalidate(data[missing_idx]);
      init_fragment_header(fragment_ptr);
      set_fragment_idx(fragment_ptr, missing_idx);
      set_orig_data_size(fragment_ptr, orig_data_size);
      set_fragment_size(fragment_ptr, blocksize);
      if (use_inline_chksum(pyeclib_handle)) {
        int chksum = crc32(0, data[missing_idx], blocksize);
        set_chksum(fragment_ptr, chksum);
      }
    } else if (missing_idx >= k) {
      int parity_idx = missing_idx - k;
      char *fragment_ptr = get_fragment_ptr_from_data_novalidate(parity[parity_idx]);
      init_fragment_header(fragment_ptr);
      set_fragment_idx(fragment_ptr, missing_idx);
      set_orig_data_size(fragment_ptr, orig_data_size);
      set_fragment_size(fragment_ptr, blocksize);
      if (use_inline_chksum(pyeclib_handle)) {
        int chksum = crc32(0, parity[parity_idx], blocksize);
        set_chksum(fragment_ptr, chksum);
      }
    } 
    j++;
  }

  /* Fill in the data fragments */
  for (i = 0; i < k; i++) {
    char *fragment_ptr = get_fragment_ptr_from_data(data[i]);
    PyList_SET_ITEM(list_of_strips, i, PY_BUILDVALUE_OBJ_LEN(fragment_ptr, fragment_size));
  }
  /* Fill in the parity fragments */
  for (i = 0; i < m; i++) {
    char *fragment_ptr = get_fragment_ptr_from_data(parity[i]);
    PyList_SET_ITEM(list_of_strips, k + i, PY_BUILDVALUE_OBJ_LEN(fragment_ptr, fragment_size));
  }
  
  goto exit;

error:
  list_of_strips = NULL;

exit:
  /* Free fragment buffers that needed to be reallocated for alignment */
  for (i = 0; i < k; i++) {
    if (realloc_bm & (1 << i)) {
      free(get_fragment_ptr_from_data_novalidate(data[i]));
    }
  }
  for (i = 0; i < m; i++) {
    if (realloc_bm & (1 << (i + k))) {
      free(get_fragment_ptr_from_data_novalidate(parity[i]));
    }
  }
  
  check_and_free_buffer(missing_idxs);
  check_and_free_buffer(data);
  check_and_free_buffer(parity);

  return list_of_strips;
}


/**
 * Obtain the metadata from a fragment.
 * 
 * @param pyeclib_obj_handle
 * @param data fragment from user to extract metadata from
 * @param data_len size in bytes of the data fragment
 * @return fragment metadata or NULL on error
 */
static PyObject *
pyeclib_c_get_metadata(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle = NULL;
  pyeclib_t* pyeclib_handle = NULL;
  char *data = NULL;                              /* param, fragment from caller */
  int data_len;                                   /* param, data len (B) */
  int metadata_len;                               /* metadata header size (B) */
  fragment_metadata_t *fragment_metadata = NULL;  /* buffer to hold metadata */
  PyObject *ret_fragment_metadata = NULL;         /* metadata object to return */

  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, GET_METADATA_ARGS, &pyeclib_obj_handle, &data, &data_len)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.get_metadata");
    return NULL;
  }
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.get_required_fragments");
    return NULL;
  }

  /* Obtain the metadata from the data and build a object to return */
  metadata_len = sizeof(fragment_metadata_t);
  fragment_metadata = (fragment_metadata_t *) alloc_zeroed_buffer(metadata_len);
  if (NULL == fragment_metadata) {
    ret_fragment_metadata = NULL;
  } else {
    get_fragment_metadata(pyeclib_handle, data, fragment_metadata);
    ret_fragment_metadata = PY_BUILDVALUE_OBJ_LEN((char*)fragment_metadata, 
                                                   metadata_len);                                                  
    check_and_free_buffer(fragment_metadata);
  }

  return ret_fragment_metadata;
}


/**
 * Confirm the health of the fragment metadata.
 *
 * TODO: Return a list containing tuples (index, problem).  An empty list means
 * everything is OK.
 * 
 * @param pyeclib_obj_handle
 * @param fragment_metadata_list list of fragment metadata headers
 * @return -1 if no errors, or the index of the first problem checksum
 */
static PyObject*
pyeclib_c_check_metadata(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle = NULL;
  pyeclib_t *pyeclib_handle = NULL;
  PyObject *fragment_metadata_list = NULL;                /* param, fragment metadata */
  fragment_metadata_t **c_fragment_metadata_list = NULL;  /* c version of metadata */
  int num_fragments;                                      /* k + m from EC algorithm */
  char **c_fragment_signatures = NULL;                    /* array of alg. signatures */
  int k, m, w;                                            /* EC algorithm params */
  int size;                                               /* size for buf allocation */
  int ret = -1;                                           /* c return value */
  PyObject *ret_obj = NULL;                               /* python long to return */
  int i = 0;                                              /* a counter */

  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, "OO", &pyeclib_obj_handle, &fragment_metadata_list)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.encode");
    return NULL;
  }
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.encode");
    return NULL;
  }
  k = pyeclib_handle->k;
  m = pyeclib_handle->m;
  w = pyeclib_handle->w;
  num_fragments = k + m;
  if (num_fragments != PyList_Size(fragment_metadata_list)) {
    PyErr_SetString(PyECLibError, "Not enough fragment metadata to perform integrity check");
    return NULL;
  }
  
  /* Allocate space for fragment signatures */
  size = sizeof(fragment_metadata_t * ) * num_fragments;
  c_fragment_metadata_list = (fragment_metadata_t**) alloc_zeroed_buffer(size);
  size = sizeof(char *) * num_fragments;
  c_fragment_signatures = (char **) alloc_zeroed_buffer(size);
  if (NULL == c_fragment_metadata_list || NULL == c_fragment_signatures) {
    goto error;
  }

  /* Populate and order the metadata */
  for (i = 0; i < num_fragments; i++) {
    PyObject *tmp_data = PyList_GetItem(fragment_metadata_list, i);
    Py_ssize_t len = 0;
    char *c_buf = NULL;
    fragment_metadata_t *fragment_metadata;
    PyBytes_AsStringAndSize(tmp_data, &(c_buf), &len);

    fragment_metadata = (fragment_metadata_t*)c_buf;
    c_fragment_metadata_list[fragment_metadata->idx] = fragment_metadata;

    if (supports_alg_sig(pyeclib_handle)) {
      c_fragment_signatures[fragment_metadata->idx] = (char*)alloc_aligned_buffer16(PYCC_MAX_SIG_LEN);
      memcpy(c_fragment_signatures[fragment_metadata->idx], fragment_metadata->signature, PYCC_MAX_SIG_LEN);
    } else {
      c_fragment_signatures[fragment_metadata->idx] = fragment_metadata->signature;
    }
  }

  /* Ensure all fragments are here and check integrity using alg signatures */
  if (supports_alg_sig(pyeclib_handle)) {
    char **parity_sigs = (char **) alloc_zeroed_buffer(sizeof(char **) * m);
    if (NULL == parity_sigs) {
      goto error;
    }
    for (i = 0; i < m; i++) {
      parity_sigs[i] = (char *) alloc_aligned_buffer16(PYCC_MAX_SIG_LEN);
      if (NULL == parity_sigs[i]) {
        int j;
        for (j = 0; j < i; j++) free(parity_sigs[j]);
        goto error;
      } else {
        memset(parity_sigs[i], 0, PYCC_MAX_SIG_LEN);
      }
    }

    /* Calculate the parity of the signatures */
    if (pyeclib_handle->type == PYECC_RS_VAND) {
      jerasure_matrix_encode(k, m, w, pyeclib_handle->matrix, 
                             c_fragment_signatures, parity_sigs, PYCC_MAX_SIG_LEN);
    } else {
      pyeclib_handle->xor_code_desc->encode(pyeclib_handle->xor_code_desc, 
                                            c_fragment_signatures,
                                            parity_sigs, 
                                            PYCC_MAX_SIG_LEN);
    }
    
    /* Compare the parity of the signatures, and the signature of the parity */
    for (i = 0; i < m; i++) {
      if (memcmp(parity_sigs[i], c_fragment_signatures[k + i], PYCC_MAX_SIG_LEN) != 0) {
        ret = i;
        break;
      }
    }

    /* Clean up memory used in algebraic signature checking */
    for (i = 0; i < m; i++) {
      free(parity_sigs[i]);
    }
    check_and_free_buffer(parity_sigs);
    for (i = 0; i < k; i++) {
      free(c_fragment_signatures[i]);
    }
  } else if (use_inline_chksum(pyeclib_handle)) {
    for (i = 0; i < num_fragments; i++) {
      if (c_fragment_metadata_list[i]->chksum_mismatch == 1) {
        ret = i;
        break;
      }
    }
  }

  /* Return index of first checksum signature error */  
  ret_obj = PyLong_FromLong((long)ret);
  goto exit;

error:
  ret_obj = NULL;

exit:
  free(c_fragment_signatures);
  free(c_fragment_metadata_list);
  
  return ret_obj;
}


static PyMethodDef PyECLibMethods[] = {
    {"init",  pyeclib_c_init, METH_VARARGS, "Initialize a new erasure encoder/decoder"},
    {"encode",  pyeclib_c_encode, METH_VARARGS, "Create parity using source data"},
    {"decode",  pyeclib_c_decode, METH_VARARGS, "Recover all lost data/parity"},
    {"reconstruct",  pyeclib_c_reconstruct, METH_VARARGS, "Recover selective data/parity"},
    {"fragments_to_string", pyeclib_c_fragments_to_string, METH_VARARGS, "Try to transform a set of fragments into original string without calling the decoder"},
    {"get_fragment_partition", pyeclib_c_get_fragment_partition, METH_VARARGS, "Parition fragments into data and parity, also returns a list of missing indexes"},
    {"get_required_fragments", pyeclib_c_get_required_fragments, METH_VARARGS, "Return the fragments required to reconstruct a set of missing fragments"},
    {"get_segment_info", pyeclib_c_get_segment_info, METH_VARARGS, "Return segment and fragment size information needed when encoding a segmented stream"},
    {"get_metadata", pyeclib_c_get_metadata, METH_VARARGS, "Get the integrity checking metadata for a fragment"},
    {"check_metadata", pyeclib_c_check_metadata, METH_VARARGS, "Check the integrity checking metadata for a set of fragments"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

MOD_INIT(pyeclib_c)
{
    PyObject *m;

    MOD_DEF(m, "pyeclib_c", NULL, PyECLibMethods);

    if (m == NULL)
      return MOD_ERROR_VAL;

    PyECLibError = PyErr_NewException("pyeclib.Error", NULL, NULL);
    if (PyECLibError == NULL) {
      fprintf(stderr, "Could not create default PyECLib exception object!\n");
      exit(2);
    }
    Py_INCREF(PyECLibError);
    PyModule_AddObject(m, "error", PyECLibError);

    return MOD_SUCCESS_VAL(m);
}
