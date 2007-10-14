// This file is the glue between Praat's script interpreter
// and external script interpreters like Python.

#include <Python.h>
#include <structmember.h>

#include "scripting.h"

#include <stdio.h>

#include "../sys/melder.h"
wchar_t * praat_getNameOfSelected (void *voidklas, int inplace);

/* Turn a Python tuple into a wchar_t list of the command and arguments. */

static wchar_t ** make_command(PyObject *args) {
	// allocate the NULL-terminated array of wide-character strings
	wchar_t **ret = (wchar_t**)calloc(PyTuple_Size(args) + 1, sizeof(wchar_t*));

	int i;
	for (i = 0; i < PyTuple_Size(args); i++) {
		PyObject *elem = PyTuple_GetItem(args, i);

		if (elem == Py_False) {
			ret[i] = wcsdup(L"false");
		} else if (elem == Py_True) {
			ret[i] = wcsdup(L"true");
		} else if (PyInt_Check(elem)) {
			long x = PyInt_AsLong(elem);
			ret[i] = (wchar_t*)calloc(30, sizeof(wchar_t));
			swprintf(ret[i], 30, L"%ld", x);
		} else if (PyFloat_Check(elem)) {
			double x = PyFloat_AsDouble(elem);
			ret[i] = (wchar_t*)calloc(30, sizeof(wchar_t));
			swprintf(ret[i], 30, L"%g", x);
		} else if (PyString_Check(elem)) {
			const char *src = PyString_AsString(elem);
			ret[i] = (wchar_t*)calloc(strlen(src)+1, sizeof(wchar_t));
			if (mbsrtowcs (ret[i], &src, strlen(src)+1, NULL) == -1)
				PyErr_SetString(PyExc_ValueError, "Wide character conversion of string argument failed.");
		} else if (PyUnicode_Check(elem)) {
			ret[i] = (wchar_t*)calloc(PyUnicode_GetSize(elem)+1, sizeof(wchar_t));
			if (PyUnicode_AsWideChar(elem, ret[i], PyUnicode_GetSize(elem) + 1) == -1)
				PyErr_SetString(PyExc_ValueError, "Wide character conversion of unicode argument failed.");
		} else {
			PyErr_SetString(PyExc_ValueError, "Only Python strings, integers, floats, True, False, and Unicode strings can be used as arguments to Praat commands.");
		}
	}

	if (PyErr_Occurred()) {
		// Free allocated memory and return NULL.
		for (i = 0; i < PyTuple_Size(args); i++)
			free(ret[i]);
		free(ret);
		return NULL;
	} else {
		return ret;
	}
}
		
/* Python methods in the praat module. */

PyObject *PyWString(wchar_t *wc) {
	// Return a PyUnicode object for this string, or if that
	// fails (don't know why it would) return a regular Python string.
	PyObject *ret = PyUnicode_FromWideChar(wc, wcslen(wc));
	if (!ret)
		ret = PyString_FromString(wc2c(wc, 1));
	return ret;
}

static wchar_t* go_internal(PyObject *args, int captureOutput) {
	int hadError;
	wchar_t **cmd;

	cmd = make_command(args);
	if (!cmd)
		return NULL;

	wchar_t *ret = scripting_executePraatCommand(cmd, captureOutput, &hadError);
	
	if (hadError) {
		char *cret = wc2c(ret, 1);
		PyErr_SetString(PyExc_Exception, cret);
		free(cret);
		return NULL;
	}

	return ret;
}

static PyObject* extfunc_go(PyObject *self, PyObject *args) {
	go_internal(args, 0);
	if (PyErr_Occurred())
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* extfunc_getString(PyObject *self, PyObject *args) {
	wchar_t *ret = go_internal(args, 1);
	if (!ret)
		return NULL;
	PyObject *ret2 = PyWString(ret);
	free(ret);
	return ret2;
}
		
static PyObject* extfunc_getNum(PyObject *self, PyObject *args) {
	wchar_t *ret = go_internal(args, 1);
	if (!ret)
		return NULL;

	float ret2;
	if (swscanf(ret, L"%g", &ret2)) {
		free(ret);
		PyObject *ret3 = PyFloat_FromDouble(ret2);
		return ret3;
	} else {
		char buf[256];
		char *cret = wc2c(ret, 1);
		sprintf(buf, "No numeric value found in Info window output: %s", cret);
		free(cret);
		PyErr_SetString(PyExc_Exception, buf);
		return NULL;
	}
}
	
static PyObject *extfunc_select(PyObject *self, PyObject *args) {
	wchar_t** command;
	const char *name;
	int haderror;

	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;

	command = (wchar_t**)calloc(3, sizeof(wchar_t*));
	command[0] = wcsdup(L"select");
	command[1] = (wchar_t*)calloc(strlen(name)+1, sizeof(wchar_t));
	command[2] = NULL;
	
	swprintf(command[1], (strlen(name)+1)*sizeof(wchar_t), L"%s", name);

	wchar_t *ret = scripting_executePraatCommand(command, 0, &haderror);
	if (haderror) {
		char *cret = wc2c(ret, 1);
		PyErr_SetString(PyExc_Exception, cret);
		free(cret);
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *extfunc_selected(PyObject *self, PyObject *args) {
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
		
	wchar_t *name = praat_getNameOfSelected(NULL, 0);
	if (name == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	
	return PyWString(name);
}

static PyMethodDef EmbMethods[] = {
    {"go", extfunc_go, METH_VARARGS,
     "Executes a Praat command, with output going to the Info window."},

    {"getString", extfunc_getString, METH_VARARGS,
     "Executes a Praat command returning the Info window output as a string."},

    {"getNum", extfunc_getNum, METH_VARARGS,
     "Executes a Praat command returning the Info window output as a float."},

    {"select", extfunc_select, METH_VARARGS,
     "Selects the Praat object of the given name."},

    {"selected", extfunc_selected, METH_VARARGS,
     "Returns the name of the selected Praat object."},

    {NULL, NULL, 0, NULL}
};

/* A special Python type for overriding sys.stdout/stderr to redirect it
 * to the info window. */

static PyObject *extfunc_InfoWindowWrite(PyObject *self, PyObject *args) {
	const char *str;
	wchar_t buffer[1024];

	if (!PyArg_ParseTuple(args, "s", &str))
		return NULL;

	memset(buffer, 0, sizeof(buffer));
	if (mbsrtowcs (buffer, &str, sizeof(buffer)/sizeof(wchar_t), NULL) == -1)
		wcscpy(buffer, L"[wide character conversion failed]");

	Melder_print (buffer);

	Py_INCREF(Py_None);
	return Py_None;
}

typedef struct {
    PyObject_HEAD
    PyObject *softspace;
} praatpy_InfoWindowStream;

static PyMethodDef praatpy_InfoWindowStream_Methods[] = {
    {"write", extfunc_InfoWindowWrite, METH_VARARGS,
     "Writes a string to the info window."
    },
    {NULL}  /* Sentinel */
};

static PyMemberDef praatpy_InfoWindowStream_Members[] = {
    {"softspace", T_OBJECT, offsetof(praatpy_InfoWindowStream, softspace), 0,
     "utility field for print statement"},
    {NULL}
};

static PyTypeObject praatpy_InfoWindowStreamObj = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "praat.InfoWindow",        /*tp_name*/
    sizeof(praatpy_InfoWindowStream), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "InfoWindowStream",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    praatpy_InfoWindowStream_Methods,             /* tp_methods */
    praatpy_InfoWindowStream_Members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};


static void initModule()  {
    PyObject* m;

    praatpy_InfoWindowStreamObj.tp_new = PyType_GenericNew;
    if (PyType_Ready(&praatpy_InfoWindowStreamObj) < 0)
        return;

    m = Py_InitModule3("praat", EmbMethods, "Praat interface module.");

    Py_INCREF(&praatpy_InfoWindowStreamObj);
    PyModule_AddObject(m, "InfoWindow", (PyObject *)&praatpy_InfoWindowStreamObj);
}

void scripting_run_python(wchar_t *script) {
	// Execute script as a Python script.
	Py_Initialize();
	initModule();
	PyRun_SimpleString("from praat import *");
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.stdout = InfoWindow()");
	PyRun_SimpleString("sys.stderr = InfoWindow()");
		
	char *cscript = wc2c(script, 0);
	PyRun_SimpleString(cscript);
	free(cscript);
		
	Py_Finalize();
}

