// This file handles Python scripting for Praat. It interfaces
// with Praat only through scripting.cpp.

#include <stdio.h>
#include <wchar.h>

#include <Python.h>
#include <structmember.h>

#include "util.h"

static wchar_t **global_argv;

static PyObject *extfunc_selected(PyObject *, PyObject *);

// We can't use the global symbols on Windows because
// we need to get a reference to the object through
// a function call.
static PyObject *g_Py_False, *g_Py_True, *g_PrPyExc;

/* Turn a Python tuple into a wchar_t list of the command and arguments. */

static wchar_t ** make_command(PyObject *args) {
	// allocate the NULL-terminated array of wide-character strings
	wchar_t **ret = (wchar_t**)calloc(PyTuple_Size(args) + 1, sizeof(wchar_t*));

	int i;
	for (i = 0; i < PyTuple_Size(args); i++) {
		PyObject *elem = PyTuple_GetItem(args, i);

		if (elem == g_Py_False) {
			ret[i] = wcsdup(L"false");
		} else if (elem == g_Py_True) {
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
				PyErr_SetString(g_PrPyExc, "Wide character conversion of string argument failed.");
		} else if (PyUnicode_Check(elem)) {
			ret[i] = (wchar_t*)calloc(PyUnicode_GetSize(elem)+1, sizeof(wchar_t));
			if (PyUnicode_AsWideChar((PyUnicodeObject*)elem, ret[i], PyUnicode_GetSize(elem) + 1) == -1)
				PyErr_SetString(g_PrPyExc, "Wide character conversion of unicode argument failed.");
		} else {
			PyErr_SetString(g_PrPyExc, "Only Python strings, integers, floats, True, False, and Unicode strings can be used as arguments to Praat commands.");
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
		PyErr_SetString(g_PrPyExc, cret);
		free(cret);
		return NULL;
	}

	return ret;
}

static PyObject* extfunc_go(PyObject *self, PyObject *args) {
	go_internal(args, 0);
	if (PyErr_Occurred())
		return NULL;

	/* Originally we just returned None, but now we
	 * return the currently selected object as a helper
	 * for commands that create objects. Here's the old code:
	 return Py_BuildValue("");
	 */
	return extfunc_selected(self, NULL);
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
		PyErr_SetString(g_PrPyExc, buf);
		return NULL;
	}
}
	
static PyObject *extfunc_do_select(PyObject *self, PyObject *args, int mode) {
	wchar_t** command;
	const char *type, *name;
	int haderror;
	int i;
	
	// This accepts a variable number of arguments.
	
	if (PyTuple_Size(args) == 0) {
		PyErr_SetString(g_PrPyExc, "You must pass at least one Praat object name to the select method.");
		return NULL;
	}
	
	for (i = 0; i < PyTuple_Size(args); i++) {
		PyObject *item = PyTuple_GetItem(args, i);
		
		// If it is a string, it is an object type and name together.
		if (PyString_Check(item)) {
			type = NULL;
			name = PyString_AsString(item);
			
		// If it is a tuple of length two, it has (type, name)
		} else if (PyTuple_Check(item) && PyTuple_Size(item) == 2) {
			if (!PyArg_ParseTuple(item, "ss", &type, &name)) 
				return NULL;

		} else {
			PyErr_SetString(g_PrPyExc, "Arguments to select must be strings like 'LongSound mysound' or tuples like ('LongSound', 'mysound').");
			return NULL;
		}
		
		command = (wchar_t**)calloc(4, sizeof(wchar_t*));		
		

		if (i == 0 && mode == 0)
			command[0] = wcsdup(L"select");
		else if (mode == 0 || mode == 1)
			command[0] = wcsdup(L"plus");
		else if (mode == 2)
			command[0] = wcsdup(L"minus");

		if (type == NULL) {
			command[1] = (wchar_t*)calloc(strlen(name)+1, sizeof(wchar_t));
			command[2] = NULL;
			swprintf(command[1], (strlen(name)+1)*sizeof(wchar_t), L"%s", name);
		} else {
			command[1] = (wchar_t*)calloc(strlen(type)+1, sizeof(wchar_t));
			command[2] = (wchar_t*)calloc(strlen(name)+1, sizeof(wchar_t));
			command[3] = NULL;
			swprintf(command[1], (strlen(type)+1)*sizeof(wchar_t), L"%s", type);
			swprintf(command[2], (strlen(name)+1)*sizeof(wchar_t), L"%s", name);
		}

		wchar_t *ret = scripting_executePraatCommand(command, 0, &haderror);
		if (haderror) {
			char *cret = wc2c(ret, 1);
			PyErr_SetString(g_PrPyExc, cret);
			free(cret);
			return NULL;
		}
	}

	return Py_BuildValue("");
}

static PyObject *extfunc_select(PyObject *self, PyObject *args) {
	return extfunc_do_select(self, args, 0);
}

static PyObject *extfunc_plus(PyObject *self, PyObject *args) {
	return extfunc_do_select(self, args, 1);
}

static PyObject *extfunc_minus(PyObject *self, PyObject *args) {
	return extfunc_do_select(self, args, 2);
}

static PyObject *extfunc_remove(PyObject *self, PyObject *args) {
	// Select...
	PyObject *none = extfunc_select(self, args);
	if (none == NULL) return NULL; // error condition
	
	// Then remove...
	scripting_executePraatCommand2(L"Remove");
	
	// And return the None value constructed for us already.
	return none;
}

static PyObject *extfunc_selected(PyObject *self, PyObject *args) {
	// args is probably never NULL coming from Python, but
	// we call this from extfunc_go and pass NULL for args
	// as a convenience.
	if (args && !PyArg_ParseTuple(args, ""))
		return NULL;
	
	// Prevents errors below if nothing is selected.
	if (!is_anything_selected())
		return Py_BuildValue("");
	
	wchar_t *name = get_name_of_selected();
	if (name == NULL)
		return Py_BuildValue("");

	char *name2 = wc2c(name, 0);
	
	char *space = strstr(name2, " ");
	if (space == NULL)
		return Py_BuildValue("");
	
	*space = 0; // split into two strings
	
	PyObject* ret = PyTuple_New(2);
	PyTuple_SetItem(ret, 0, PyString_FromString(name2));
	PyTuple_SetItem(ret, 1, PyString_FromString(strdup(space+1)));
	return ret;
}

static PyObject *extfunc_argv(PyObject *self, PyObject *args) {
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	if (global_argv == NULL)
		return Py_BuildValue("");

	PyObject* ret = PyList_New(0);
	for (int i = 0; global_argv[i]; i++)
		PyList_Append(ret, PyWString(global_argv[i]));
		
	return ret;
}

static PyMethodDef EmbMethods[] = {
    {"go", extfunc_go, METH_VARARGS,
     "Executes a Praat command, with output going to the Info window."},

    {"getString", extfunc_getString, METH_VARARGS,
     "Executes a Praat command returning the Info window output as a string."},

    {"getNum", extfunc_getNum, METH_VARARGS,
     "Executes a Praat command returning the Info window output as a float."},

    {"select", extfunc_select, METH_VARARGS,
     "Selects the Praat object (or multiple objects). Pass either a name like 'LongSound mysound' or the type and name as a tuple like select(('Sound', 'mysound1'), ('Sound', 'mysound2'))."},

    {"plus", extfunc_plus, METH_VARARGS,
     "Adds the Praat object (or multiple objects) to the current selection. Pass either a name like 'LongSound mysound' or the type and name as a tuple like plus(('Sound', 'mysound1'), ('Sound', 'mysound2'))."},

    {"minus", extfunc_minus, METH_VARARGS,
     "Deselects the Praat object (or multiple objects). Pass either a name like 'LongSound mysound' or the type and name as a tuple like minus(('Sound', 'mysound1'), ('Sound', 'mysound2'))."},

    {"remove", extfunc_remove, METH_VARARGS,
     "Removes the Praat object (or multiple objects) from the Praat objects list. Pass either a name like 'LongSound mysound' or the type and name as a tuple like remove(('Sound', 'mysound1'), ('Sound', 'mysound2'))."},

    {"selected", extfunc_selected, METH_VARARGS,
     "Returns the type and name of the selected Praat object, e.g. (type, name) = selected()."},

    {"getargv", extfunc_argv, METH_VARARGS,
     "Returns a list of the command-line arguments, including the script name itself as the first item in the list."},

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

	write_to_info_window(buffer);

	return Py_BuildValue("");
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
    
    g_PrPyExc = PyErr_NewException("praat.PraatPyException", NULL, NULL);
}

void scripting_run_python(wchar_t *script, wchar_t **argv) {
	// Execute script as a Python script.
	global_argv = argv;
	Py_Initialize();
	initModule();
	g_Py_False = PyBool_FromLong(0);
	g_Py_True = PyBool_FromLong(1);
	PyRun_SimpleString("import praat");
	PyRun_SimpleString("praat.argv = praat.getargv()");
	PyRun_SimpleString("from praat import *");
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.stdout = InfoWindow()");
	PyRun_SimpleString("sys.stderr = InfoWindow()");
	PyRun_SimpleString("sys.path = ['.'] + sys.path");
		
	char *cscript = wc2c(script, 0);
	PyRun_SimpleString(cscript);
	free(cscript);
		
	Py_Finalize();
}

