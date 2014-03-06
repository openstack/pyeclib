#include <Python.h>

/*
 * for older versions of Python, we don't use PyBytes, but keep PyString
 * and don't use Capsule but CObjects
 */
#if PY_VERSION_HEX < 0x02070000
#ifndef PyBytes_Check
#define PyBytes_Check PyString_Check
#define PyBytes_Size PyString_Size
#define PyBytes_AsString PyString_AsString
#define PyBytes_AS_STRING PyString_AS_STRING
#define PyBytes_GET_SIZE PyString_GET_SIZE
#endif
#ifndef PyCapsule_New
#define PyCapsule_New PyCObject_FromVoidPtrAndDesc
#define PyCapsule_CheckExact PyCObject_Check
#define PyCapsule_GetPointer(o, n) PyCObject_AsVoidPtr((o))
#endif
#endif

