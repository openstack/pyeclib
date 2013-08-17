#include<Python.h>
#include<reed_sol.h>
#include<cauchy.h>
#include<jerasure.h>
#include<liberation.h>
#include<galois.h>

static PyObject *PyECLibError;

typedef enum { PYECC_RS_VAND, PYECC_RS_CAUCHY_ORIG, PYECC_NUM_TYPES, PYECC_NOT_FOUND } pyeclib_type_t;
const char *pyeclib_type_str[] = { "rs_vand", "rs_cauchy_orig" };

#define PYECC_FLAGS_MASK          0x1
#define PYECC_FLAGS_READ_VERIFY   0x1

#define PYECC_HANDLE_NAME "pyeclib_handle"

#define PYECC_HEADER_MAGIC  0xb0c5ecc

/*
 * This structure must be word-aligned 
 * (should detect this and adjust)
 * otherwise the compiler will try to pad 
 * it.
 */
typedef struct fragment_header_s {
  int magic;
  int idx;
  int size;
  int pad;
} fragment_header_t;

typedef struct pyeclib_s {
  int k;
  int m;
  int w;
  int *matrix;
  int *bitmatrix;
  int **schedule;
  pyeclib_type_t type;  
} pyeclib_t;

/*
 * Validate the basic erasure code parameters 
 */
static int validate_args(int k, int m, int w, pyeclib_type_t type)
{

  switch (type) {
    case PYECC_RS_CAUCHY_ORIG:
      {
        if (w < 31 && (k+m) > (1 << w)) {
          return -1;
        }
      }
    case PYECC_RS_VAND:
    default:
      {
        int max_symbols;
      
        if (w != 8 || w != 16 || w != 32) {
          return -1;
        }
        max_symbols = 1 << w;
        if ((k+m) > max_symbols) {
          return -1;
        }
      }
  }
  return 0;
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

static void pyeclib_destructor(PyObject *obj)
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

static PyObject *
pyeclib_init(PyObject *self, PyObject *args)
{
  pyeclib_t *pyeclib_handle;
  PyObject* pyeclib_obj_handle;
  int k, m, w;
  const char *type_str;
  pyeclib_type_t type;
  int *matrix;
  int *bitmatrix;
  int **schedule;

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

  switch (type) {
    case PYECC_RS_CAUCHY_ORIG:
      matrix = cauchy_original_coding_matrix(k, m, w);
      bitmatrix = jerasure_matrix_to_bitmatrix(k, m, w, matrix);
      schedule = jerasure_smart_bitmatrix_to_schedule(k, m, w, bitmatrix);
      break;
    case PYECC_RS_VAND:
    default:
      matrix = reed_sol_vandermonde_coding_matrix(k, m, w);
      break;
  }

  if (matrix == NULL) {
    PyErr_SetString(PyECLibError, "Could not create a generator matrix in pyeclib.init");
    return NULL;
  }
  
  pyeclib_handle = (pyeclib_t*)malloc(sizeof(pyeclib_t));
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "malloc() returned NULL in pyeclib.init");
    return NULL;
  }

  pyeclib_handle->k = k;
  pyeclib_handle->m = m;
  pyeclib_handle->w = w;
  pyeclib_handle->matrix = matrix;
  pyeclib_handle->type = type;

  if (type == PYECC_RS_CAUCHY_ORIG) {
    pyeclib_handle->bitmatrix = bitmatrix;
    pyeclib_handle->schedule = schedule;
  }

  pyeclib_obj_handle = PyCapsule_New(pyeclib_handle, PYECC_HANDLE_NAME, pyeclib_destructor);

  if (pyeclib_obj_handle == NULL) {
    PyErr_SetString(PyECLibError, "Could not encapsulate pyeclib_handle into Python object in pyeclib.init");
    return NULL;
  }

  Py_INCREF(pyeclib_obj_handle);

  return pyeclib_obj_handle;
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

  buf = (char*)malloc(size);
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
  fragment_header_t *header;

  buf -= sizeof(fragment_header_t);

  header = (fragment_header_t*)buf;
  
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
    exit(2);
  }

  return buf;
}

static 
void set_fragment_idx(char *buf, int idx)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    fprintf(stderr, "Invalid fragment header (idx check)!\n");
    exit(2);
  }

  header->idx = idx;
}

static 
void set_fragment_size(char *buf, int size)
{
  fragment_header_t* header = (fragment_header_t*)buf;

  if (header->magic != PYECC_HEADER_MAGIC) {
    fprintf(stderr, "Invalid fragment header (size check)!\n");
    exit(2);
  }

  header->size = size;
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
void free_fragment_buffer(char *buf)
{
  fragment_header_t *header;
  if (buf == NULL) {
    return;
  }

  buf -= sizeof(fragment_header_t);
  
  header = (fragment_header_t*)buf;
  if (header->magic != PYECC_HEADER_MAGIC) {
    fprintf(stderr, "Invalid fragment header!\n");
    exit(2);
  }
  free(buf); 
}

static PyObject *
pyeclib_encode(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle;
  pyeclib_t *pyeclib_handle;
  char *data;
  int data_len;
  int blocksize, residual, padding;
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

  /*
   * Compute the blocksize 
   *
   * TODO: blocksize MUST BE WORD ALIGNED!!!!
   */
  blocksize = data_len / pyeclib_handle->k;

  /*
   * Since the buffer is probably not aligned, 
   * the last block will probably need padding
   */
  residual = data_len % blocksize;
  padding = blocksize - residual;

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
    set_fragment_size(fragment, data_len > blocksize ? blocksize : data_len);
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
    set_fragment_size(fragment, blocksize);
    if (encoded_parity[i] == NULL) {
      PyErr_SetString(PyECLibError, "Could not allocate memory in pyeclib.encode");
      return NULL;
    }
  }

  switch (pyeclib_handle->type) {
    case PYECC_RS_CAUCHY_ORIG:
      {
        int packetsize = blocksize / pyeclib_handle->w;
        jerasure_schedule_encode(pyeclib_handle->k, pyeclib_handle->m, pyeclib_handle->w, pyeclib_handle->schedule, data_to_encode, encoded_parity, blocksize, packetsize);
        break;
      }
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
    PyList_SET_ITEM(list_of_strips, i, Py_BuildValue("s#", fragment_ptr, fragment_size));
    free_fragment_buffer(data_to_encode[i]);
  }
  free(data_to_encode);
  
  for (i=0; i < pyeclib_handle->m; i++) {
    char *fragment_ptr = get_fragment_ptr_from_data(encoded_parity[i]);
    int fragment_size = blocksize+sizeof(fragment_header_t);
    set_fragment_idx(fragment_ptr, pyeclib_handle->k+i);
    PyList_SET_ITEM(list_of_strips, pyeclib_handle->k + i, Py_BuildValue("s#", fragment_ptr, fragment_size));
    free_fragment_buffer(encoded_parity[i]);
  }
  free(encoded_parity);

  return list_of_strips;
}

static PyObject *
pyeclib_decode(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle;
  PyObject *data_list;
  PyObject *parity_list;
  PyObject *missing_idx_list;
  pyeclib_t *pyeclib_handle;
  int blocksize;
  char **data;
  char **parity;
  int *missing_idxs;
  PyObject *list_of_strips;
  int i, j;
  
  if (!PyArg_ParseTuple(args, "OOOOi", &pyeclib_obj_handle, &data_list, &parity_list, &missing_idx_list, &blocksize)) {
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
  
  missing_idxs = (int*)malloc(sizeof(int)*(pyeclib_handle->k+pyeclib_handle->m));
  for (i = 0; i < (pyeclib_handle->k+pyeclib_handle->m); i++) {
    PyObject *obj_idx = PyList_GetItem(missing_idx_list, i); 
    long idx = PyLong_AsLong(obj_idx);

    missing_idxs[i] = (int)idx;  

    if (idx < 0) {
      break;
    }
  }

  data = (char**)malloc(sizeof(char*) * pyeclib_handle->k); 
  if (data == NULL) {
    PyErr_SetString(PyECLibError, "Could not allocate memory for data buffers");
    return NULL;
  }
  for (i=0; i < pyeclib_handle->k; i++) {
    PyObject *tmp_data = PyList_GetItem(data_list, i);
    long len = 0;
    PyString_AsStringAndSize(tmp_data, &(data[i]), &len);
    if (validate_fragment(data[i]) < 0) {
      int is_missing = 0;
      j = 0;
      while (missing_idxs[j] >= 0) {
        if (missing_idxs[j] == i) {
          is_missing = 1;
          break;
        }
        j++;
      }
      if (is_missing) {
        // TODO: Really need a clean way to manage the headers + fragment payload
        data[i] += sizeof(fragment_header_t);
        continue;
      }
    }
    data[i] = get_fragment_data(data[i]);
  }

  parity = (char**)malloc(sizeof(char*) * pyeclib_handle->m); 
  if (parity == NULL) {
    PyErr_SetString(PyECLibError, "Could not allocate memory for parity buffers");
    return NULL;
  }

  for (i=0; i < pyeclib_handle->m; i++) {
    PyObject *tmp_parity = PyList_GetItem(parity_list, i);
    long len = 0;
    PyString_AsStringAndSize(tmp_parity, &(parity[i]), &len);
    if (validate_fragment(parity[i]) < 0) {
      int is_missing = 0;
      j = 0;
      while (missing_idxs[j] >= 0) {
        if (missing_idxs[j] == pyeclib_handle->k + i) {
          is_missing = 1;
          break;
        }
        j++;
      }
      if (is_missing) {
        // TODO: Really need a clean way to manage the headers + fragment payload
        parity[i] += sizeof(fragment_header_t);
        continue;
      }
    }
    parity[i] = get_fragment_data(parity[i]);
  }
  
  switch (pyeclib_handle->type) {
    case PYECC_RS_CAUCHY_ORIG:
      {
        int packetsize = blocksize / pyeclib_handle->w;
        jerasure_schedule_decode_lazy(pyeclib_handle->k, pyeclib_handle->m, pyeclib_handle->w, pyeclib_handle->bitmatrix, missing_idxs, data, parity, blocksize, packetsize, 1);
        break;
      }
    case PYECC_RS_VAND:
    default:
      jerasure_matrix_decode(pyeclib_handle->k, pyeclib_handle->m, pyeclib_handle->w, pyeclib_handle->matrix, 1, missing_idxs, data, parity, blocksize-sizeof(fragment_header_t));
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
      set_fragment_size(fragment_ptr, blocksize-sizeof(fragment_header_t));
        
    } else if (missing_idx >= pyeclib_handle->k) {
      int parity_idx = missing_idx - pyeclib_handle->k;
      char *fragment_ptr = get_fragment_ptr_from_data_novalidate(parity[parity_idx]);
      init_fragment_header(fragment_ptr);
      set_fragment_idx(fragment_ptr, missing_idx);
      set_fragment_size(fragment_ptr, blocksize-sizeof(fragment_header_t));
    } 
    j++;
  }

  /*
   * Fill in the data fragments
   */
  for (i=0; i < pyeclib_handle->k; i++) {
    char *fragment_ptr = get_fragment_ptr_from_data(data[i]);
    PyList_SET_ITEM(list_of_strips, i, Py_BuildValue("s#", fragment_ptr, blocksize));
  }
  /*
   * Fill in the parity fragments
   */
  for (i=0; i < pyeclib_handle->m; i++) {
    char *fragment_ptr = get_fragment_ptr_from_data(parity[i]);
    PyList_SET_ITEM(list_of_strips, pyeclib_handle->k + i, Py_BuildValue("s#", fragment_ptr, blocksize));
  }

  free(missing_idxs);

  return list_of_strips;
}

static PyMethodDef PyECLibMethods[] = {
    {"init",  pyeclib_init, METH_VARARGS, "Initialize a new erasure encoder/decoder"},
    {"encode",  pyeclib_encode, METH_VARARGS, "Create parity using source data"},
    {"decode",  pyeclib_decode, METH_VARARGS, "Recover lost data/parity"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initpyeclib(void)
{
    PyObject *m;
    m = Py_InitModule("pyeclib", PyECLibMethods);
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

