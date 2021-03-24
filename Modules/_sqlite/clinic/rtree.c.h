/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(create_query_function__doc__,
"create_query_function($module, connection, name, func, /)\n"
"--\n"
"\n"
"Creates an R*Tree query callback. Non-standard.");

#define CREATE_QUERY_FUNCTION_METHODDEF    \
    {"create_query_function", (PyCFunction)(void(*)(void))create_query_function, METH_FASTCALL, create_query_function__doc__},

static PyObject *
create_query_function_impl(PyObject *module, pysqlite_Connection *connection,
                           const char *name, PyObject *func);

static PyObject *
create_query_function(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    pysqlite_Connection *connection;
    const char *name;
    PyObject *func;

    if (!_PyArg_CheckPositional("create_query_function", nargs, 3, 3)) {
        goto exit;
    }
    connection = (pysqlite_Connection *)args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("create_query_function", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(args[1], &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    func = args[2];
    return_value = create_query_function_impl(module, connection, name, func);

exit:
    return return_value;
}
/*[clinic end generated code: output=d56a291307a39ca5 input=a9049054013a1b77]*/
