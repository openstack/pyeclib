#ifndef PY3_COMPAT_H
#define PY3_COMPAT_H

#if PY_MAJOR_VERSION >= 3

#define IS_PY3K

/* Define some aliases for the removed PyInt_* functions */
#define PyInt_Check(op) PyLong_Check(op)
#define PyInt_FromString PyLong_FromString
#define PyInt_FromUnicode PyLong_FromUnicode
#define PyInt_FromLong PyLong_FromLong
#define PyInt_FromSize_t PyLong_FromSize_t
#define PyInt_FromSsize_t PyLong_FromSsize_t
#define PyInt_AsLong PyLong_AsLong
#define PyInt_AsSsize_t PyLong_AsSsize_t
#define PyInt_AsUnsignedLongMask PyLong_AsUnsignedLongMask
#define PyInt_AsUnsignedLongLongMask PyLong_AsUnsignedLongLongMask
#define PyInt_AS_LONG PyLong_AS_LONG
#define PyNumber_Int PyNumber_Long

/* Weakrefs flags changed in 3.x */
#define Py_TPFLAGS_HAVE_WEAKREFS 0

/* Module init function returns new module instance. */
#define MODINIT_RETURN(x) return x
#define MODINIT_DEFINE(mod_name) PyMODINIT_FUNC PyInit_##mod_name (void)
#define DECREF_MOD(mod) Py_DECREF (mod)

/* Type header differs. */
#define TYPE_HEAD(x,y) PyVarObject_HEAD_INIT(x,y)

/* Text interface. Use unicode strings. */
#define Text_Type PyUnicode_Type
#define Text_Check PyUnicode_Check
#define Text_FromUTF8 PyUnicode_FromString
#define Text_FromUTF8AndSize PyUnicode_FromStringAndSize
#define Text_FromFormat PyUnicode_FromFormat
#define Text_GetSize PyUnicode_GetSize
#define Text_GET_SIZE PyUnicode_GET_SIZE

/* Binary interface. Use bytes. */
#define Bytes_Type PyBytes_Type
#define Bytes_Check PyBytes_Check
#define Bytes_Size PyBytes_Size
#define Bytes_AsString PyBytes_AsString
#define Bytes_AsStringAndSize PyBytes_AsStringAndSize
#define Bytes_FromStringAndSize PyBytes_FromStringAndSize
#define Bytes_AS_STRING PyBytes_AS_STRING
#define Bytes_GET_SIZE PyBytes_GET_SIZE

#define IsTextObj(x) (PyUnicode_Check(x) || PyBytes_Check(x))

/* String interface. Use unicode strings */
#define PyString_FromString PyUnicode_FromString
#define PyString_FromString PyUnicode_FromString

#ifdef PyString_FromStringAndSize
#undef PyString_FromStringAndSize
#endif
#define PyString_FromStringAndSize PyUnicode_FromStringAndSize

#ifdef PyString_AsString
#undef PyString_AsString
#endif
#define PyString_AsString PyUnicode_AsUTF8

#ifdef PyString_AsStringAndSize
#undef PyString_AsStringAndSize
#endif
static inline int PyString_AsStringAndSize(PyObject* obj, char** buf,
                                           Py_ssize_t* psize) {
    if (PyUnicode_Check(obj)) {
        *buf = PyUnicode_AsUTF8AndSize(obj, psize);
        return *buf == NULL ? -1 : 0;
    } else if (PyBytes_Check(obj)) {
        return PyBytes_AsStringAndSize(obj, buf, psize);
    }
    PyErr_SetString(PyExc_TypeError, "Expecting str or bytes");
    return -1;
}

/* Renamed builtins */
#define BUILTINS_MODULE "builtins"
#define BUILTINS_UNICODE "str"
#define BUILTINS_UNICHR "chr"

#endif /* PY_MAJOR_VERSION */

#endif /* PY3_COMPAT_H */
