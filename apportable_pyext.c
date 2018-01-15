
#define PY_SSIZE_T_CLEAN

#include "stdlib.h"
#include "Python.h"
#include "bytesobject.h"

#include "apportable.h"

#include <stdio.h>

// #include <iconv.h>


struct module_state {
    PyObject *error;
    apportable_t apportable;
};

#if PY_VERSION_HEX >= 0x03000000
# define GETSTATE(self) ((struct module_state*)PyModule_GetState((self)))
#else
# define GETSTATE(self) (&_state)
static struct module_state _state;
#endif


char * hexdump(void * p, size_t count)
{
	unsigned char * m = (unsigned char *) p;
	char * s = calloc(3, count);
	for (unsigned int i = 0; i < count; i++) {
		sprintf(s + 3*i, "%02x", ((m + i))[0]);
		s[3*i + 2] = ' ';
	}
	s[3*count - 1] = 0;
	return s;
}


char * hexstr(char * p)
{
	size_t l;
	l = strlen(p);
	return hexdump(p, l + 1);
}


char * whexstr(wchar_t * p)
{
	size_t l;
	l = wcslen(p);
	return hexdump(p, sizeof(wchar_t) * (l + 1));
}


/* Python Unicode string object -> wchar_t * */
wchar_t * _appoext_pyobywstr (PyObject * self, PyObject * str)
{
	wchar_t * wstr;
	Py_ssize_t str_l;

#if PY_VERSION_HEX >= 0x03020000
	str_l = PyUnicode_GetLength(str);
	if (!(wstr = PyUnicode_AsWideCharString(str, NULL)))
		return NULL;
#else
	Py_ssize_t wstr_l;

	str_l = PyUnicode_GetSize(str);        /* DEPRECATED in Python 3.2+ */
	wstr_l = sizeof(wchar_t) * (str_l + 1);
	if (!(wstr = PyMem_Malloc(wstr_l)))
		return NULL;
	memset(wstr, 0, wstr_l);
	if (-1 == (wstr_l = PyUnicode_AsWideChar((PyUnicodeObject *) str, wstr, wstr_l))) {
		PyMem_Free(wstr);
		return NULL;
	}
	wstr[wstr_l] = 0;
#endif

	return wstr;   /* to PyMem_Free() */
}


/* take a Python Unicode string object, and get a UTF-8 copy */
char * _appoext_pyobyutf8 (PyObject * self, PyObject * str)
{
	PyObject * bytes;
	char * s, * ret;
	size_t s_l;

	if (!(bytes = PyUnicode_AsUTF8String(str)))
		return NULL;
	if (!(s = PyBytes_AsString(bytes))) {
		return NULL;
	}
	s_l = strlen(s);
	if (!(ret = PyMem_Malloc(s_l + 1)))
		return NULL;
	memcpy(ret, s, s_l);
	ret[s_l] = 0;
	return ret;
}

int _selftest (PyObject * self)
{
	struct module_state *st;
	st = GETSTATE(self);
	apportable a = &(st->apportable);

	// wchar_t * t = L"äβ©☃☂";
	// wchar_t * x = "❤"
	// "\xc3\xa4\xce\xb2\xc2\xa9\xe2\x98\x83\xe2\x98\x82"; /* UTF-16 */
	char * tt = "\xc3\xa4\xce\xb2\xe2\x9d\xa4\xc2\xa9\xe2\x98\x83\xe2\x98\x82";
 	char * r;
	wchar_t * wr;
	char * r2;
	PyObject * o = PyUnicode_FromString(tt);

	
	r = _appoext_pyobyutf8(self, o);
	printf("utf-8  : %zu '%s'\n", strlen(r), r);
	
	for (int i = 0; i < 5; i++) {

		wr = _appoext_pyobywstr(self, o);
		printf("wchar_t: %zu %s\n", wcslen(wr), whexstr(wr));

		r2 = a->wutf8(a, wr);
		printf("wutf8(): %zu %s\n", strlen(r2), hexstr(r2));

		o = PyUnicode_FromString(r2);
	}

	return 0;

}

static PyObject *
appoext_selftest (PyObject * self, PyObject * args)
{
	_selftest(self);
	Py_RETURN_NONE;
}


static PyObject *
appoext_strndup (PyObject * self, PyObject * args)
{
	PyObject * str;
	Py_ssize_t size;
	struct module_state *st;
	char * s, * ret;
	PyObject * pyres;

 	if (!PyArg_ParseTuple(args, "Un", &str, &size)) {
  		return NULL;
	}
	st = GETSTATE(self);
	s = _appoext_pyobyutf8(self, str);
	ret = st->apportable._strndup(&(st->apportable), s, size);

	pyres = PyUnicode_FromString(ret);
	free(ret);
	return pyres;
}




static PyObject *
appoext_wcsndup (PyObject * self, PyObject * args)
{
	PyObject * str;
	Py_ssize_t size;
	wchar_t * wstr;
	struct module_state *st;
	char * ret;
	PyObject * pyres;

 	if (!PyArg_ParseTuple(args, "Un", &str, &size)) {
  		return NULL;
	}
	st = GETSTATE(self);
	wstr = _appoext_pyobywstr(self, str);
	ret = (char *) st->apportable._wcsndup(&(st->apportable), wstr, size);
	PyMem_Free(wstr);

	pyres = PyUnicode_FromWideChar((wchar_t *) ret, wcslen((wchar_t *) ret));
	free(ret);
	return pyres;
}


static PyObject *
appoext_wutf8 (PyObject * self, PyObject * args)
{
	PyObject * str;
	wchar_t * wstr;
	struct module_state *st;
	char * ret;
	PyObject * pyres;

 	if (!PyArg_ParseTuple(args, "U", &str)) {
  		return NULL;
	}
	st = GETSTATE(self);
	wstr = _appoext_pyobywstr(self, str);
	ret = st->apportable.wutf8(&(st->apportable), wstr);

	pyres = PyUnicode_FromString(ret);
	free(ret);
	return pyres;
}


static PyObject *
appoext_uwchar_t (PyObject * self, PyObject * args)
{
	PyObject * str;
	char * cstr;
	struct module_state *st;
	wchar_t * ret;
	PyObject * pyres;

 	if (!PyArg_ParseTuple(args, "U", &str)) {
  		return NULL;
	}
	st = GETSTATE(self);
	cstr = _appoext_pyobyutf8(self, str);
	ret = st->apportable.uwchar_t(&(st->apportable), cstr);

	pyres = PyUnicode_FromWideChar((wchar_t *) ret, wcslen((wchar_t *) ret));
	free(ret);
	return pyres;
}



static PyObject *
appoext_whereis (PyObject * self, PyObject * args)
{
	struct module_state *st;
	PyObject * osp, * obin;
	char * searchpath, * bin;
	int execonly;
	char * ret;
	PyObject * pyres;

 	if (!PyArg_ParseTuple(args, "UUb", &osp, &obin, &execonly)) {
  		return NULL;
	}
	st = GETSTATE(self);
	searchpath = _appoext_pyobyutf8(self, osp);
	bin = _appoext_pyobyutf8(self, obin);

	ret = st->apportable.whereis(&(st->apportable), searchpath, bin, execonly);
	if (!ret) {
		Py_RETURN_NONE;
	}
	pyres = PyUnicode_FromString(ret);
	free(ret);
	return pyres;
}


static PyObject *
appoext_progfile (PyObject * self, PyObject * args)
{
  const char * library_name;
	struct module_state *st;
	char * ret;
	PyObject * pyres;

 	if (!PyArg_ParseTuple(args, "z", &library_name)) {
  		return NULL;
	}
	st = GETSTATE(self);

	ret = st->apportable.progfile(&(st->apportable), library_name);
	// pyres = Py_BuildValue("s", ret);
	pyres = PyUnicode_FromString(ret);
	free(ret);
	return pyres;
}


static PyObject *
appoext_pathexp (PyObject * self, PyObject * args)
{
	PyObject * ot, * olb;
	const char * template, * library_path;
	struct module_state *st;
	char * ret;
	PyObject * pyres;

	if (!PyArg_ParseTuple(args, "UU", &ot, &olb)) {
    	return NULL;
  	}
	st = GETSTATE(self);
	template = _appoext_pyobyutf8(self, ot);
	library_path = _appoext_pyobyutf8(self, olb);

	ret = st->apportable.pathexp(&(st->apportable), template, library_path);
	pyres = PyUnicode_FromString(ret);
	free(ret);
	return pyres;
}



static PyObject *
appoext_ugetenv (PyObject * self, PyObject * args)
{
	struct module_state *st;
	PyObject * oenv;
	char * env;
	char * ret;
	PyObject * pyres;

 	if (!PyArg_ParseTuple(args, "U", &oenv)) {
  		return NULL;
	}
	st = GETSTATE(self);
	env = _appoext_pyobyutf8(self, oenv);

	ret = st->apportable.ugetenv(&(st->apportable), env);
	if (!ret) {
		Py_RETURN_NONE;
	}
	pyres = PyUnicode_FromString(ret);
	free(ret);
	return pyres;
}


static PyObject *
appoext_wugetenv (PyObject * self, PyObject * args)
{
	struct module_state *st;
	PyObject * oenv;
	wchar_t * env;
	char * ret;
	PyObject * pyres;

 	if (!PyArg_ParseTuple(args, "U", &oenv)) {
  		return NULL;
	}
	st = GETSTATE(self);
	env = _appoext_pyobywstr(self, oenv);

	ret = st->apportable.wugetenv(&(st->apportable), env);
	if (!ret) {
		Py_RETURN_NONE;
	}
	pyres = PyUnicode_FromString(ret);
	free(ret);
	return pyres;
}




static PyMethodDef apportable_methods[] = {
	{"selftest", appoext_selftest, METH_VARARGS, NULL},
	{"strndup", appoext_strndup, METH_VARARGS, NULL},
	{"wcsndup", appoext_wcsndup, METH_VARARGS, NULL},
	{"wutf8", appoext_wutf8, METH_VARARGS, NULL},
	{"uwchar_t", appoext_uwchar_t, METH_VARARGS, NULL},
    {"progfile", appoext_progfile, METH_VARARGS, NULL},
    {"pathexp", appoext_pathexp, METH_VARARGS, NULL},
    {"whereis", appoext_whereis, METH_VARARGS, NULL},
    {"ugetenv", appoext_ugetenv, METH_VARARGS, NULL},
    {"wugetenv", appoext_wugetenv, METH_VARARGS, NULL},
    { NULL, NULL, 0, NULL }
};


#if PY_MAJOR_VERSION >= 3
static int apportable_traverse(PyObject *self, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(self)->error);
    return 0;
}

static int apportable_clear(PyObject *self) {
    Py_CLEAR(GETSTATE(self)->error);
    return 0;
}

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT, "apportable", NULL, sizeof(struct module_state),
      apportable_methods, NULL, apportable_traverse, apportable_clear, NULL
};

#define INITERROR return NULL

PyMODINIT_FUNC
PyInit_apportable(void)

#else /* PY_MAJOR_VERSION >= 3 */
#define INITERROR return

void
initapportable(void)
#endif
{
	struct module_state *st;
	

#if PY_MAJOR_VERSION >= 3
  PyObject *module = PyModule_Create(&moduledef);
#else
  PyObject *module = Py_InitModule("apportable", apportable_methods);
#endif

  if (module == NULL)
    INITERROR;
  st = GETSTATE(module);
  apportable_init(&(st->apportable), 1);

  st->error = PyErr_NewException("apportable.Error", NULL, NULL);
  if (st->error == NULL) {
    Py_DECREF(module);
    INITERROR;
  }

#if PY_MAJOR_VERSION >= 3
  return module;
#endif
}



/* Not for release */

// PyErr_Format(PyExc_ValueError, "X %zd", str_l);
// return NULL;

