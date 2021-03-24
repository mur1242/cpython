/*
 * rtree.c - R*Tree support
 *
 * Written by Erlend E. Aasland <erlend.aasland@protonmail.com>
 */

#include "Python.h"

// pysqlite includes
#include "connection.h"

#include "sqlite3.h"


/*[clinic input]
module _sqlite3_rtree
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=c24e56f4ea363aac]*/
#include "clinic/rtree.c.h"

PyDoc_STRVAR(rtree_doc,
"R*Tree support for the sqlite3 module.\n\n"
"This module is an implementation detail. Please do not use it directly.");

static void
decref_context(void *obj)
{
    Py_DECREF(obj);
}

static PyObject *
value_to_object(sqlite3_value *value)
{
    PyObject *item = NULL;

    switch (sqlite3_value_type(value)) {
    case SQLITE_INTEGER:
        item = PyLong_FromLongLong(sqlite3_value_int64(value));
        break;
    case SQLITE_FLOAT:
        item = PyFloat_FromDouble(sqlite3_value_double(value));
        break;
    case SQLITE_TEXT: {
        const char *text = (const char*)sqlite3_value_text(value);
        Py_ssize_t size = sqlite3_value_bytes(value);
        item = PyUnicode_FromStringAndSize(text, size);
        break;
    }
    case SQLITE_BLOB: {
        const void *blob = sqlite3_value_blob(value);
        Py_ssize_t size = sqlite3_value_bytes(value);
        item = PyBytes_FromStringAndSize(blob, size);
        break;
    }
    case SQLITE_NULL:
        item = Py_NewRef(Py_None);
    }

    return item;
}

static PyObject *
build_params(int n_params, sqlite3_value **values)
{
    PyObject *tuple = PyTuple_New(n_params);
    if (tuple == NULL) {
        return NULL;
    }

    for (int i = 0; i < n_params; i++) {
        PyObject *item = value_to_object(values[i]);
        if (item == NULL) {
            Py_DECREF(tuple);
            return NULL;
        }
        PyTuple_SET_ITEM(tuple, i, item);
    }

    return tuple;
}

static int
query_callback(sqlite3_rtree_query_info *info)
{
    PyObject *parameters = NULL;
    PyObject *coordinates = NULL;
    int rc = SQLITE_ERROR;

    assert(info != NULL);
    PyGILState_STATE tstate = PyGILState_Ensure();

    /* Build tuple of parameters given to the SQL function. */
#if SQLITE_VERSION_NUMBER >= 3008011
    parameters = build_params(info->nParam, info->apSqlParam);
    if (parameters == NULL) {
        goto error;
    }
#else
    parameters = PyTuple_New(info->nParam);
    if (parameters == NULL) {
        goto error;
    }
    for (int i = 0; i < info->nParam; i++) {
        PyObject *param = PyFloat_FromDouble(info->aParam[i]);
        if (PyTuple_SetItem(parameters, i, param) < 0) {
            goto error;
        }
    }
#endif

    /* Build tuple of coordinates to check. */
    coordinates = PyTuple_New(info->nCoord);
    if (coordinates == NULL) {
        goto error;
    }
    for (int i = 0; i < info->nCoord; i++) {
        PyObject *coord = PyFloat_FromDouble(info->aCoord[i]);
        PyTuple_SET_ITEM(coordinates, i, coord);
    }

    /* Call UDF */
    PyObject *callback = (PyObject *)info->pContext;
    int num_queued = info->mxLevel + 1;
    PyObject *queue = PyList_New(num_queued);
    if (queue == NULL) {
        goto error;
    }
    for (int i = 0; i < num_queued; i++) {
        PyObject *num_pending = PyLong_FromUnsignedLong(info->anQueue[i]);
        PyList_SET_ITEM(queue, i, num_pending);
    }
    PyObject *args[] = {
        Py_NewRef(coordinates),
        Py_NewRef(parameters),
        queue,
        Py_NewRef((info->pUser != NULL) ? info->pUser : Py_None),
        PyLong_FromLong(info->iLevel),
        PyLong_FromLong(info->mxLevel),
        PyLong_FromLongLong(info->iRowid),
        PyFloat_FromDouble(info->rParentScore),
        PyLong_FromLongLong(info->eParentWithin),
        NULL
    };
    PyObject *kwnames = PyTuple_Pack(7,
                                     PyUnicode_FromString("num_queued"),
                                     PyUnicode_FromString("context"),
                                     PyUnicode_FromString("level"),
                                     PyUnicode_FromString("max_level"),
                                     PyUnicode_FromString("rowid"),
                                     PyUnicode_FromString("parent_score"),
                                     PyUnicode_FromString("parent_visibility"));
    PyObject *res = PyObject_Vectorcall(callback,
                                        args,
                                        PyVectorcall_NARGS(2),
                                        kwnames);
    Py_XDECREF(kwnames);
    Py_ssize_t nargs = Py_ARRAY_LENGTH(args);
    for (int i = 0; i < nargs; i++) {
        Py_XDECREF(args[i]);
    }

    /* Parse response tuple: (visibility, score, context) */
    if (res == NULL) {
        goto error;
    }
    if (!PyTuple_Check(res) || PyTuple_Size(res) != 3) {
        Py_DECREF(res);
        goto error;
    }
    /* Note: borrowed refs. */
    PyObject *visibility = PyTuple_GET_ITEM(res, 0);
    PyObject *score = PyTuple_GET_ITEM(res, 1);
    PyObject *context = PyTuple_GET_ITEM(res, 2);

    /* If this is the first call, set UDF context pointer and cleaup.
     * The cleaup will run when the last callback for this query is done. */
    if (info->pUser == NULL && context != NULL) {
        info->pUser = Py_NewRef(context);
        info->xDelUser = &decref_context;
    }

    /* Set out parameters. */
    info->eWithin = PyLong_AsLong(visibility);
    info->rScore = PyFloat_AsDouble(score);
    rc = SQLITE_OK;
    Py_DECREF(res);

error:
    Py_XDECREF(parameters);
    Py_XDECREF(coordinates);
    PyGILState_Release(tstate);
    return rc;
}

/*[clinic input]
_sqlite3_rtree.create_query_function as create_query_function

    connection: object(type='pysqlite_Connection *')
    name: str
    func: object
    /

Creates an R*Tree query callback. Non-standard.
[clinic start generated code]*/

static PyObject *
create_query_function_impl(PyObject *module, pysqlite_Connection *connection,
                           const char *name, PyObject *func)
/*[clinic end generated code: output=c4b3dca900866d95 input=1b975d4544230826]*/
{
    if (connection == NULL) {
        return NULL;
    }

    int rc;
    if (func == Py_None) {
        rc = sqlite3_rtree_query_callback(connection->db, name, 0, 0, 0);
    }
    else {
        rc = sqlite3_rtree_query_callback(connection->db,
                                          name,
                                          &query_callback,
                                          Py_NewRef(func),
                                          &decref_context);
    }

    if (rc == SQLITE_NOMEM) {
        return PyErr_NoMemory();
    }
    if (rc != SQLITE_OK) {
        PyErr_SetString(connection->Error, sqlite3_errmsg(connection->db));
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyMethodDef rtree_methods[] = {
    CREATE_QUERY_FUNCTION_METHODDEF
    {NULL},
};

static struct PyModuleDef rtree_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_sqlite3_rtree",
    .m_doc = rtree_doc,
    .m_methods = rtree_methods,
};

PyMODINIT_FUNC
PyInit__sqlite3_rtree(void)
{
    return PyModuleDef_Init(&rtree_module);
}
