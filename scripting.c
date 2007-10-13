// This file is the glue between Praat's script interpreter
// and external script interpreters like Python.

#include <Python.h>
#include <structmember.h>

#include <stdio.h>

#include "../sys/melder.h"
int praat_executeCommand (void *me, const char *command);
char * praat_getNameOfSelected (void *voidklas, int inplace);

/* Interface into Praat. */

static void *current_interpreter = NULL; // global state; bad programming style, yes

static char *scripting_praat_executeCommand (const char *command, int divert, int *haderror) {
	// This runs a Praat Script command already formed in the command
	// argument. If divert is true, the output to the info window is diverted
	// and captured, and returned by this function (as a newly-allocated buffer
	// to be freed by the caller). haderror is set on returning to whether an
	// error ocurred, and if so the error is returned as a newly allocated
	// buffer. If no error occurs and divert is false, NULL is returned.

	MelderStringA value = { 0, 0, NULL };

	if (divert) Melder_divertInfo (&value);
	praat_executeCommand (current_interpreter, command);
	if (divert) Melder_divertInfo (NULL);
	
	if (Melder_hasError()) {
		MelderStringA_free(&value);
		*haderror = 1;
		char *ret = strdup (Melder_getError ());
		Melder_clearError ();
		return ret;
	}
	
	*haderror = 0;
	
	if (divert) {
		if (value.string) {
			char *ret = strdup (value.string);
			MelderStringA_free(&value);
			return ret;
		} else {
			char *ret = (char*)malloc(1);
			*ret = 0;
			return ret;
		}
	} else {
		return NULL;
	}
}

/* Make a Praat command from a Python tuple (command name and arguments). */

static int make_command(PyObject *args, char *ret) {
	// ret is a buffer allocated by the caller.
	// TODO: Check buffer overrun.

	char buffer[256];
	int i;

	strcpy(ret, "");

	for (i = 0; i < PyTuple_Size(args); i++) {
		if (i > 0) strcat(ret, " ");

		PyObject *elem = PyTuple_GetItem(args, i);

		if (elem == Py_False) {
			strcpy(buffer, "false");
		} else if (elem == Py_True) {
			strcpy(buffer, "true");
		} else if (PyInt_Check(elem)) {
			long x = PyInt_AsLong(elem);
			if (PyErr_Occurred())
				return 0;
			sprintf(buffer, "%ld", x);
		} else if (PyFloat_Check(elem)) {
			double x = PyFloat_AsDouble(elem);
			sprintf(buffer, "%g", x);
		} else if (PyString_Check(elem)) {
			strcpy(buffer, PyString_AsString(elem));
		} else {
			PyErr_SetString(PyExc_ValueError, "Only Python strings, integers, floats, True, and False can be used as arguments to Praat commands.");
			return 0;
		}

		// For all but the last argument, put quotes around
		// the argument if it contains spaces or quotes.
		int needs_escape = 0;
		if (i > 0 && i < PyTuple_Size(args) - 1) {
			if (strstr(buffer, " ") || strstr(buffer, "\""))
				needs_escape = 1;
		}

		if (!needs_escape) {
			strcat(ret, buffer);
		} else {
			strcat(ret, "\"");
			char *r = ret + strlen(ret);
			int i;
			for (i = 0; i < strlen(buffer); i++) {
				*(r++) = buffer[i];
				if (buffer[i] == '"') // escape quotes by doubling them
					*(r++) = '"';
			}
			*(r++) = 0;
			strcat(ret, "\"");
		}
	}

	return 1;
}
		
/* Python methods in the praat module. */

static char* go_internal(PyObject *args, int captureOutput) {
	int hadError;
	char cmd[256];

	if (!make_command(args, cmd))
		return NULL;

	char *ret = scripting_praat_executeCommand(cmd, captureOutput, &hadError);

	if (hadError) {
		PyErr_SetString(PyExc_Exception, ret);
		return NULL;
	}

	return ret;
}

static PyObject* extfunc_go(PyObject *self, PyObject *args) {
	go_internal(args, 0);
	if (PyErr_Occurred())
		return NULL;
	return Py_None;
}

static PyObject* extfunc_getString(PyObject *self, PyObject *args) {
	char *ret = go_internal(args, 1);
	if (!ret)
		return NULL;
	PyObject *ret2 = PyString_FromString(ret);
	free(ret);
	return ret2;
}
		
static PyObject* extfunc_getNum(PyObject *self, PyObject *args) {
	char *ret = go_internal(args, 1);
	if (!ret)
		return NULL;

	float ret2;
	if (sscanf(ret, "%g", &ret2)) {
		free(ret);
		PyObject *ret3 = PyFloat_FromDouble(ret2);
		return ret3;
	} else {
		char buf[256];
		sprintf(buf, "No numeric value found in Info window output: %s", ret);
		free(ret);
		PyErr_SetString(PyExc_Exception, buf);
		return NULL;
	}
}
	
static PyObject *extfunc_select(PyObject *self, PyObject *args) {
	char command[256];
	const char *name;
	int haderror;

	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;

	strcpy(command, "select ");
	strcat(command, name);

	char *ret = scripting_praat_executeCommand(command, 0, &haderror);
	if (haderror) {
		PyErr_SetString(PyExc_Exception, ret);
		free(ret);
		return NULL;
	}

	return Py_None;
}

static PyObject *extfunc_selected(PyObject *self, PyObject *args) {
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
		
	char *name = praat_getNameOfSelected(NULL, 0);
	if (name == NULL)
		return Py_None;
	
	return PyString_FromString(name);
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
	char *str;

	if (!PyArg_ParseTuple(args, "s", &str))
		return NULL;

	Melder_print (str);

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

/* The main entry point. */

void scripting_run_praat_script(void *interpreter, char *script) {
	current_interpreter = interpreter;

	if (strncmp(script, "#lang=python", 12) == 0) {
		// Execute script as a Python script.
		Py_Initialize();
		initModule();
		PyRun_SimpleString("from praat import *");
		PyRun_SimpleString("import sys");
		PyRun_SimpleString("sys.stdout = InfoWindow()");
		PyRun_SimpleString("sys.stderr = InfoWindow()");
		PyRun_SimpleString(script);
		Py_Finalize();
	} else {
		Melder_print ("Unrecognized language in #lang= line in script. Use \"#lang=python\".\n");
	}

	current_interpreter = NULL;
}

