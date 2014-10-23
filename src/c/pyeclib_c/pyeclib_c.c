/* 
 * Copyright (c) 2013-2014, Kevin Greenan (kmgreen2@gmail.com)
 * Copyright (c) 2014, Tushar Gohad (tusharsg@gmail.com)
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

#include <erasurecode.h>
#include <erasurecode_helpers.h>
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


static PyObject *PyECLibError;

typedef struct pyeclib_byte_range {
  uint64_t offset;
  uint64_t length;
} pyeclib_byte_range_t;

/**
 * Prototypes for Python/C API methods
 */
static PyObject * pyeclib_c_init(PyObject *self, PyObject *args);
static void pyeclib_c_destructor(PyObject *obj);
static PyObject * pyeclib_c_get_segment_info(PyObject *self, PyObject *args);
static PyObject * pyeclib_c_encode(PyObject *self, PyObject *args);
static PyObject * pyeclib_c_reconstruct(PyObject *self, PyObject *args);
static PyObject * pyeclib_c_decode(PyObject *self, PyObject *args);
static PyObject * pyeclib_c_get_metadata(PyObject *self, PyObject *args);
static PyObject * pyeclib_c_check_metadata(PyObject *self, PyObject *args);

/**
 * Constructor method for creating a new pyeclib object using the given parameters.
 *
 * @param k integer number of data elements
 * @param m integer number of checksum elements
 * @param w integer word size in bytes
 * @param backend_id erasure coding backend
 * @return pointer to PyObject or NULL on error
 */
static PyObject *
pyeclib_c_init(PyObject *self, PyObject *args)
{
  pyeclib_t *pyeclib_handle = NULL;
  PyObject *pyeclib_obj_handle = NULL;
  int k, m, hd=0;
  int use_inline_chksum = 0, use_algsig_chksum = 0;
  const ec_backend_id_t backend_id;

  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, "iii|iii", &k, &m, &backend_id, &hd, &use_inline_chksum, &use_algsig_chksum)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.init");
    return NULL;
  }

  /* Allocate and initialize the pyeclib object */
  pyeclib_handle = (pyeclib_t *) alloc_zeroed_buffer(sizeof(pyeclib_t));
  if (NULL == pyeclib_handle) {
    goto error;
  }

  pyeclib_handle->ec_args.k = k;
  pyeclib_handle->ec_args.m = m;
  pyeclib_handle->ec_args.hd = hd;
  pyeclib_handle->ec_args.ct = use_inline_chksum ? CHKSUM_CRC32 : CHKSUM_NONE;

  pyeclib_handle->ec_desc = liberasurecode_instance_create(backend_id, &(pyeclib_handle->ec_args));  
  if (pyeclib_handle->ec_desc <= 0) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to liberasurecode_instance_create");
    goto error; 
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
    goto error;
  } else {
    Py_INCREF(pyeclib_obj_handle);
  }
  
  goto exit;

error:
  check_and_free_buffer(pyeclib_handle);
  pyeclib_obj_handle = NULL;

exit:  
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
  min_segment_size = liberasurecode_get_minimum_encode_size(pyeclib_handle->ec_desc);

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
    aligned_data_len = liberasurecode_get_aligned_data_size(pyeclib_handle->ec_desc, data_len);

    /* aligned_data_len is guaranteed to be divisible by k */
    fragment_size = aligned_data_len / pyeclib_handle->ec_args.k;

    /* Segment size is the user-provided segment size */
    segment_size = data_len;
    last_fragment_size = fragment_size;
    last_segment_size = segment_size;
  } else {
    /*
     * There will be at least 2 segments, where the last exceeeds
     * the minimum segment size.
     */

    aligned_segment_size = liberasurecode_get_aligned_data_size(pyeclib_handle->ec_desc, segment_size);

    /* aligned_data_len is guaranteed to be divisible by k */
    fragment_size = aligned_segment_size / pyeclib_handle->ec_args.k;

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
    
    aligned_segment_size = liberasurecode_get_aligned_data_size(pyeclib_handle->ec_desc, last_segment_size);
     
    /* Compute the last fragment size from the last segment size */
    last_fragment_size = aligned_segment_size / pyeclib_handle->ec_args.k;
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
  char **encoded_data = NULL;     /* array of k data buffers */
  char **encoded_parity = NULL;     /* array of m parity buffers */
  PyObject *list_of_strips = NULL;  /* list of encoded strips to return */
  char *data;                       /* param, data buffer to encode */
  int data_len;                     /* param, length of data buffer */
  uint64_t fragment_len;            /* length, in bytes of the fragments */
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

  if (liberasurecode_encode(pyeclib_handle->ec_desc,
        data, data_len, &encoded_data, &encoded_parity, &fragment_len) < 0) {

    PyErr_SetString(PyECLibError, "Encountered error in liberasurecode_encode");
    return NULL; 
  }
  
  /* Create the python list of fragments to return */
  list_of_strips = PyList_New(pyeclib_handle->ec_args.k + pyeclib_handle->ec_args.m);
  if (NULL == list_of_strips) {
    PyErr_SetString(PyECLibError, "Error allocating python list in encode");
    return NULL; 
  }
  
  /* Add data fragments to the python list to return */
  for (i = 0; i < pyeclib_handle->ec_args.k; i++) {
    PyList_SET_ITEM(list_of_strips, i, 
                    PY_BUILDVALUE_OBJ_LEN(encoded_data[i], fragment_len));
  }
  
  /* Add parity fragments to the python list to return */
  for (i = 0; i < pyeclib_handle->ec_args.m; i++) {
    PyList_SET_ITEM(list_of_strips, pyeclib_handle->ec_args.k + i, 
                    PY_BUILDVALUE_OBJ_LEN(encoded_parity[i], fragment_len));
  }

  liberasurecode_encode_cleanup(pyeclib_handle->ec_desc, encoded_data, encoded_parity);
  
  return list_of_strips;
}


/**
 * Return a list of lists with valid rebuild indexes given an EC algorithm
 * and a list of missing indexes.
 *
 * @param pyeclib_obj_handle
 * @param reconstruct_list list of missing fragments to reconstruct
 * @param exclude_list list of fragments to exclude from reconstruction
 * @return a list of lists of indexes to rebuild data from
 */
static PyObject *
pyeclib_c_get_required_fragments(PyObject *self, PyObject *args)
{
  PyObject *pyeclib_obj_handle = NULL;
  pyeclib_t *pyeclib_handle = NULL;
  PyObject *reconstruct_list = NULL;        /* list of missing fragments to reconstruct */
  PyObject *exclude_list = NULL;        /* list of fragments to exclude from reconstruction */
  PyObject *fragment_idx_list = NULL;   /* list of req'd indexes to return */
  int *c_reconstruct_list = NULL;           /* c-array of missing indexes */
  int *c_exclude_list = NULL;           /* c-array of missing indexes */
  int num_missing;                      /* size of passed in missing list */
  int num_exclude;                      /* size of passed in exclude list */
  int i = 0;                            /* counters */
  int k, m;                             /* EC algorithm parameters */
  int *fragments_needed = NULL;         /* indexes of xor code fragments */
  int ret;                              /* return value for xor code */

  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, "OOO", &pyeclib_obj_handle, &reconstruct_list, &exclude_list)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.get_required_fragments");
    return NULL;
  }
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.get_required_fragments");
    return NULL;
  }
  k = pyeclib_handle->ec_args.k;
  m = pyeclib_handle->ec_args.m;

  /* Generate -1 terminated c-array and bitmap of missing indexes */
  num_missing = (int) PyList_Size(reconstruct_list);
  c_reconstruct_list = (int *) alloc_zeroed_buffer((num_missing + 1) * sizeof(int));
  if (NULL == c_reconstruct_list) {
    return NULL;
  }
  c_reconstruct_list[num_missing] = -1;
  for (i = 0; i < num_missing; i++) {
    PyObject *obj_idx = PyList_GetItem(reconstruct_list, i);
    long idx = PyLong_AsLong(obj_idx);
    c_reconstruct_list[i] = (int) idx;
  }

  num_exclude = (int) PyList_Size(exclude_list);
  c_exclude_list = (int *) alloc_zeroed_buffer((num_exclude + 1) * sizeof(int));
  if (NULL == c_exclude_list) {
    goto exit; 
  }
  c_exclude_list[num_exclude] = -1;
  for (i = 0; i < num_exclude; i++) {
    PyObject *obj_idx = PyList_GetItem(exclude_list, i);
    long idx = PyLong_AsLong(obj_idx);
    c_exclude_list[i] = (int) idx;
  }

  fragments_needed = alloc_zeroed_buffer(sizeof(int) * (k + m));
  if (NULL == fragments_needed) {
    goto exit;
  }

  ret = liberasurecode_fragments_needed(pyeclib_handle->ec_desc, c_reconstruct_list, 
                                        c_exclude_list, fragments_needed);
  if (ret < 0) {
    PyErr_Format(PyECLibError, "Not enough fragments for pyeclib.get_required_fragments!");
    goto exit; 
  }
   
  /* Post-process into a Python list */
  fragment_idx_list = PyList_New(0);
  if (NULL == fragment_idx_list) {
    goto exit;
  }

  for (i = 0; fragments_needed[i] > -1; i++) {
    PyList_Append(fragment_idx_list, Py_BuildValue("i", fragments_needed[i]));
  }

exit:
  check_and_free_buffer(c_reconstruct_list);
  check_and_free_buffer(c_exclude_list);
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
  PyObject *fragments = NULL;           /* param, list of fragments */
  PyObject *reconstructed = NULL;       /* reconstructed object to return */
  char * c_reconstructed = NULL;        /* C string of reconstructed fragment */
  int fragment_len;                     /* param, size in bytes of fragment */
  int num_fragments;                    /* number of fragments passed in */
  char **c_fragments = NULL;            /* C array containing the fragment payloads */
  int destination_idx;                  /* param, index to reconstruct */
  int ret;                              /* decode matrix creation return val */
  int i = 0;                            /* a counter */

  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, "OOii", &pyeclib_obj_handle, &fragments, 
                                        &fragment_len, &destination_idx)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.reconstruct");
    return NULL;
  }
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.reconstruct");
    return NULL;
  }

  /* Pre-processing Python data structures */
  if (!PyList_Check(fragments)) {
    PyErr_SetString(PyECLibError, "Invalid structure passed in for fragment list");
    return NULL;
  }

  num_fragments = PyList_Size(fragments);

  c_fragments = (char **) alloc_zeroed_buffer(sizeof(char *) * num_fragments);
  if (NULL == c_fragments) {
    goto error;
  }

  c_reconstructed = (char*) alloc_zeroed_buffer(sizeof(char) * fragment_len);
  if (NULL == c_fragments) {
    goto error;
  }

  /* Put the fragments into an array of C strings */
  for (i = 0; i < num_fragments; i++) {
    PyObject *tmp_data = PyList_GetItem(fragments, i);
    Py_ssize_t len = 0;
    PyBytes_AsStringAndSize(tmp_data, &(c_fragments[i]), &len);
  }
  
  ret = liberasurecode_reconstruct_fragment(pyeclib_handle->ec_desc, 
                                            c_fragments, 
                                            num_fragments, 
                                            fragment_len, 
                                            destination_idx, 
                                            c_reconstructed); 
  if (ret < 0) {
    PyErr_SetString(PyECLibError, "Error calling liberasurecode_reconstruct_fragment");
    reconstructed = NULL;
  } else {
    reconstructed = PY_BUILDVALUE_OBJ_LEN(c_reconstructed, fragment_len);
  }
  
  goto out;

error:
  reconstructed = NULL;
  
out:
  check_and_free_buffer(c_fragments);
  check_and_free_buffer(c_reconstructed);

  return reconstructed;
}

/**
 * Decode a set of fragments into the original string
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
  PyObject *fragments = NULL;             /* param, list of missing indexes */
  PyObject *ret_payload = NULL;           /* object to store original payload or ranges of payload */
  PyObject *ranges = NULL;                /* a list of tuples that represent byte ranges */
  pyeclib_byte_range_t *c_ranges = NULL;  /* the byte ranges */
  int num_ranges = 0;                     /* number of specified ranges */
  int fragment_len;                       /* param, size in bytes of fragment */
  char **c_fragments = NULL;              /* k length array of data buffers */
  int num_fragments;                      /* param, number of fragments */
  char *c_orig_payload = NULL;            /* buffer to store original payload in */
  uint64_t range_payload_size = 0;        /* length of buffer used to store byte ranges */
  uint64_t orig_data_size = 0;            /* data size in bytes ,from fragment hdr */
  int i = 0;                              /* counters */
  int ret = 0;

  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, "OOi|O", &pyeclib_obj_handle, &fragments, &fragment_len, &ranges)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.decode");
    return NULL;
  }
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.decode");
    return NULL;
  }
  if (!PyList_Check(fragments)) {
    PyErr_SetString(PyECLibError, "Invalid structure passed in for available fragments in pyeclib.decode");
    return NULL;
  }
  
  num_fragments = PyList_Size(fragments);

  if (ranges) {
    num_ranges = PyList_Size(ranges);
  }

  if (pyeclib_handle->ec_args.k > num_fragments) {
    PyErr_SetString(PyECLibError, "The fragment list does not have enough entries in pyeclib.decode");
    return NULL;
  }
    
  if (num_ranges > 0) {
    c_ranges = (pyeclib_byte_range_t*)malloc(sizeof(pyeclib_byte_range_t) * num_ranges);
    if (NULL == c_ranges) {
        PyErr_SetString(PyECLibError, "Could not allocate memory in pyeclib_c.decode");
        goto error;
    }
    for (i = 0; i < num_ranges; i++) {
      PyObject *tuple = PyList_GetItem(ranges, i);
      if (PyTuple_Size(tuple) == 2) {
        PyObject *py_begin = PyTuple_GetItem(tuple, 0);
        PyObject *py_end = PyTuple_GetItem(tuple, 1);

        if (!PyLong_Check(py_begin) || !PyLong_Check(py_end)) {
          PyErr_SetString(PyECLibError, "Invalid range passed to pyeclib.decode");
          goto error;
        }

        c_ranges[i].offset = PyLong_AsLong(py_begin);
        c_ranges[i].length = PyLong_AsLong(py_end) - c_ranges[i].offset + 1;
        range_payload_size += c_ranges[i].length;
      } else {
        PyErr_SetString(PyECLibError, "Invalid range passed to pyeclib.decode");
        goto error;
      }
    }
  }
  
  c_fragments = (char **) alloc_zeroed_buffer(sizeof(char *) * num_fragments);
  if (NULL == c_fragments) {
    goto error;
  }
  
  /* Put the fragments into an array of C strings */
  for (i = 0; i < num_fragments; i++) {
    PyObject *tmp_data = PyList_GetItem(fragments, i);
    Py_ssize_t len = 0;
    PyBytes_AsStringAndSize(tmp_data, &(c_fragments[i]), &len);
  }
  
  ret = liberasurecode_decode(pyeclib_handle->ec_desc, 
                            c_fragments,
                            num_fragments,
                            fragment_len,
                            &c_orig_payload,
                            &orig_data_size);

  if (num_ranges == 0) {
    ret_payload = PY_BUILDVALUE_OBJ_LEN(c_orig_payload, orig_data_size);
  } else {
    ret_payload = PyList_New(num_ranges);
    if (NULL == ret_payload) {
        PyErr_SetString(PyECLibError, "Could not alloc list for range payloads in pyeclib.decode");
        goto error;
    }
    range_payload_size = 0;
    for (i = 0; i < num_ranges; i++) {
      /* Check that range is within the original buffer */
      if (c_ranges[i].offset + c_ranges[i].length > orig_data_size) {
        PyErr_SetString(PyECLibError, "Invalid range passed to pyeclib.decode");
        goto error;
      }
      PyList_SET_ITEM(ret_payload, i,
        PY_BUILDVALUE_OBJ_LEN(c_orig_payload + c_ranges[i].offset, c_ranges[i].length));
    }
  }

  goto exit;

error:
  ret_payload = NULL;

exit:
  check_and_free_buffer(c_fragments);
  check_and_free_buffer(c_ranges);
  liberasurecode_decode_cleanup(pyeclib_handle->ec_desc, c_orig_payload); 

  return ret_payload;
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
  char *fragment = NULL;                            /* param, fragment from caller */
  fragment_metadata_t c_fragment_metadata;          /* structure to hold metadata */
  PyObject *fragment_metadata = NULL;               /* metadata object to return */
  int ret;

  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, GET_METADATA_ARGS, &pyeclib_obj_handle, &fragment)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.get_metadata");
    return NULL;
  }
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.get_required_fragments");
    return NULL;
  }

  ret = liberasurecode_get_fragment_metadata(fragment, &c_fragment_metadata);

  if (ret < 0) {
    fragment_metadata = NULL;
    PyErr_SetString(PyECLibError, "Failed to get metadata in pyeclib.get_metadata");
  } else {
    fragment_metadata = PY_BUILDVALUE_OBJ_LEN((char*)&c_fragment_metadata, 
                                                 sizeof(fragment_metadata_t));                                                  
  }

  return fragment_metadata;
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
  char **c_fragment_metadata_list = NULL;                 /* c version of metadata */
  int fragment_metadata_size;                             /* size of the metadata payload */
  int num_fragments;                                      /* k + m from EC algorithm */
  int k, m;                                               /* EC algorithm params */
  int size;                                               /* size for buf allocation */
  int ret = -1;                                           /* c return value */
  PyObject *ret_obj = NULL;                               /* python long to return */
  int i = 0;                                              /* a counter */

  /* Obtain and validate the method parameters */
  if (!PyArg_ParseTuple(args, "OOi", &pyeclib_obj_handle, &fragment_metadata_list, &fragment_metadata_size)) {
    PyErr_SetString(PyECLibError, "Invalid arguments passed to pyeclib.encode");
    return NULL;
  }
  pyeclib_handle = (pyeclib_t*)PyCapsule_GetPointer(pyeclib_obj_handle, PYECC_HANDLE_NAME);
  if (pyeclib_handle == NULL) {
    PyErr_SetString(PyECLibError, "Invalid handle passed to pyeclib.encode");
    return NULL;
  }
  k = pyeclib_handle->ec_args.k;
  m = pyeclib_handle->ec_args.m;
  num_fragments = k + m;
  if (num_fragments != PyList_Size(fragment_metadata_list)) {
    PyErr_SetString(PyECLibError, "Not enough fragment metadata to perform integrity check");
    return NULL;
  }
  
  /* Allocate space for fragment signatures */
  size = sizeof(char * ) * num_fragments;
  c_fragment_metadata_list = (char **) alloc_zeroed_buffer(size);
  if (NULL == c_fragment_metadata_list) {
    goto error;
  }

  /* Populate the metadata into a C array */
  for (i = 0; i < num_fragments; i++) {
    PyObject *tmp_data = PyList_GetItem(fragment_metadata_list, i);
    Py_ssize_t len = 0;
    PyBytes_AsStringAndSize(tmp_data, &(c_fragment_metadata_list[i]), &len);
  }
  
  ret = liberasurecode_verify_stripe_metadata(pyeclib_handle->ec_desc, c_fragment_metadata_list, 
                                              num_fragments);
        
  ret_obj = PyLong_FromLong((long)ret);

  goto exit;

error:
  ret_obj = NULL;

exit:
  free(c_fragment_metadata_list);
  
  return ret_obj;
}


static PyMethodDef PyECLibMethods[] = {
    {"init",  pyeclib_c_init, METH_VARARGS, "Initialize a new erasure encoder/decoder"},
    {"encode",  pyeclib_c_encode, METH_VARARGS, "Create parity using source data"},
    {"decode",  pyeclib_c_decode, METH_VARARGS, "Recover all lost data/parity"},
    {"reconstruct",  pyeclib_c_reconstruct, METH_VARARGS, "Recover selective data/parity"},
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
