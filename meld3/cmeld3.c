#include <Python.h>

static PyObject *PySTR__class__, *PySTR__dict__, *PySTR_children;
static PyObject *PySTRattrib, *PySTRparent, *PySTR_MELD_ID;
static PyObject *PySTRtag, *PySTRtext, *PySTRtail, *PySTRstructure;
static PyObject *PySTRget_html_start, *PySTRget_html_tail;
static PyObject *PySTRget_html_finish;
static PyObject *PySTR__dict__;

static PyObject*
clone(PyObject *node, PyObject *parent)
{
    PyObject *klass;
    PyObject *children;
    PyObject *text;
    PyObject *tail;
    PyObject *tag;
    PyObject *attrib;
    PyObject *structure;
    PyObject *dict;

    PyObject *newdict;
    PyObject *newchildren;
    PyObject *attrib_copy;
    PyObject *element;

    if (!(klass = PyObject_GetAttr(node, PySTR__class__))) return NULL;
    if (!(dict = PyObject_GetAttr(node, PySTR__dict__))) return NULL;

    if (!(children = PyDict_GetItem(dict, PySTR_children))) return NULL;
    if (!(tag = PyDict_GetItem(dict, PySTRtag))) return NULL;
    if (!(attrib = PyDict_GetItem(dict, PySTRattrib))) return NULL;

    if (!(text = PyDict_GetItem(dict, PySTRtext))) {
	text = Py_None;
    }
    if (!(tail = PyDict_GetItem(dict, PySTRtail))) {
	tail = Py_None;
    }
    if (!(structure = PyDict_GetItem(dict, PySTRstructure))) {
	structure = Py_None;
    }

    Py_INCREF(text);
    Py_INCREF(tail);
    Py_INCREF(tag);
    Py_INCREF(structure);

    if (!(newdict = PyDict_New())) return NULL;
    if (!(newchildren = PyList_New(0))) return NULL;

    attrib_copy = PyDict_Copy(attrib);

    PyDict_SetItem(newdict, PySTR_children, newchildren);
    PyDict_SetItem(newdict, PySTRattrib, attrib_copy);
    PyDict_SetItem(newdict, PySTRtext, text);
    PyDict_SetItem(newdict, PySTRtail, tail);
    PyDict_SetItem(newdict, PySTRtag, tag);
    PyDict_SetItem(newdict, PySTRstructure, structure);
    
    /* element = self.__class__(self.tag, self.attrib.copy()) */
    /* element.tail = self.tail */
    /* element.text = self.text */

    if (!(element = PyInstance_NewRaw(klass, newdict))) return NULL;
 
    /* if parent is not None:
       parent._children.append(element)
       element.parent = parent */

    PyObject *pchildren;
    
    if (parent != Py_None) {
        if (!(pchildren = PyObject_GetAttr(parent, PySTR_children))) return NULL;
	if (PyList_Append(pchildren, element)) return NULL;
	if (PyObject_SetAttr(element, PySTRparent, parent)) return NULL;
    }

    /* for child in self._children:
       child.clone(element) */

    int len, i;
    len = PyList_Size(children);
    if (len < 0) return NULL;

    PyObject *child;

    for (i = 0; i < len; i++) {
	if (!(child = PyList_GetItem(children, i))) {
	    return NULL;
	}
	clone(child, element);
	}
    
    return element;

}

static PyObject*
clonehandler(PyObject *self, PyObject *args)
{
    PyObject *node, *parent;
	
    if (!PyArg_ParseTuple(args, "OO:clone", &node, &parent)) {
	return NULL;
    }
    
    return clone(node, parent);
}

static char clonehandler_doc[] =
"clone(node, parent=None)\n			\
\n\
Return a clone of the meld3 node named by node.  If parent is not None, \n\
append the clone to the parent.\n";

static PyObject*
getiterator(PyObject *node, PyObject *list) {
    if (PyList_Append(list, node) == -1) {
	return NULL;
    }
    Py_INCREF(node);
    PyObject *children;
    PyObject *child;

    if (!(children = PyObject_GetAttr(node, PySTR_children))) {
	return NULL;
    }

    int len, i;
    len = PyList_Size(children);
    if (len < 0) {
	return NULL;
    }

    for (i = 0; i < len; i++) {
	if (!(child = PyList_GetItem(children, i))) {
	    return NULL;
	}
        getiterator(child, list);
	}
    
    return list;
}

static PyObject*
getiteratorhandler(PyObject *self, PyObject *args)
{
    PyObject *node;
	
    if (!PyArg_ParseTuple(args, "O:getiterator", &node)) {
	return NULL;
    }
    PyObject *list;
    PyObject *result;
    if (!(list = PyList_New(0))) {
	return NULL;
    }
    result = getiterator(node, list);
    if (result == NULL) {
	PyList_SetSlice(list, 0, PyList_GET_SIZE(list), (PyObject *)NULL);
	Py_DECREF(list);
    }
    return result;
}

static char getiteratorhandler_doc[] =
"getiterator(node)\n\
\n\
Returns an iterator for the node.\n";

static char* _MELD_ID = "{http://www.plope.com/software/meld3}id";

static PyObject*
findmeld(PyObject *node, PyObject *name) {
    PyObject *attrib = PyObject_GetAttr(node, PySTRattrib);
    PyObject *meldid = PyDict_GetItem(attrib, PySTR_MELD_ID);
    PyObject *result = Py_None;

    if (meldid != NULL) {
        if ( PyUnicode_Compare(meldid, name) == 0) {
	    result = node;
	}
    }

    if (result == Py_None) {
	int len, i;
	PyObject *children = PyObject_GetAttr(node, PySTR_children);
	len = PyList_Size(children);
	for (i = 0; i < len; i++) {
	    PyObject *child = PyList_GetItem(children, i);
	    Py_INCREF(child);
	    result = findmeld(child, name);
            if (result != Py_None) {
		break;
	    }
	    Py_DECREF(child);
	}
    }

    return result;
    
}

static PyObject*
findmeldhandler(PyObject *self, PyObject *args)
{
    PyObject *node, *name;
	
    if (!PyArg_ParseTuple(args, "OO:findmeld", &node, &name)) {
	return NULL;
    }
    PyObject *result = findmeld(node, name);
    Py_INCREF(result);
    return result;
}

static char findmeldhandler_doc[] =
"findmeld(node, meldid)\n\
\n\
Return a meld node or None.\n";

static PyObject*
write_html(PyObject *write, PyObject *node, PyObject *namespaces) {
    int len, i;

    /* xmlns_items = [] # new namespaces in this scope */
    PyObject *xmlns_items = PyList_New(0);

    PyObject *start = NULL;
    start = PyObject_CallMethodObjArgs(node, PySTRget_html_start, namespaces, 
				       xmlns_items, NULL);
    if (!start) return NULL;

    if ((len = PyList_Size(start)) < 0) return NULL;
    for (i = 0; i < len; i++) {
	PyObject *str = PyList_GET_ITEM(start, i);
	PyObject_CallFunctionObjArgs(write, str, NULL);
    }

    PyObject *children = PyObject_GetAttr(node, PySTR_children);
    if ((len = PyList_Size(children)) < 0) return NULL;
    for (i = 0; i < len; i++) {
	/*self.write_html(write, child, namespaces)*/
	PyObject *child = PyList_GET_ITEM(children, i);
	write_html(write, child, namespaces);
    }

    /*fprintf(stderr, "%s:%s:%d\n", __FILE__,__FUNCTION__,__LINE__);
      fflush(stderr);*/

    /*node.write_html_finish(write, namespaces, xmlns_items)*/

    PyObject *finish, *tag;
    tag = PyObject_GetAttr(node, PySTRtag);
    if (PyUnicode_Check(tag) || PyString_Check(tag)) {
	
	finish = PyObject_CallMethodObjArgs(node, PySTRget_html_finish,
					    namespaces, xmlns_items, NULL);
	if (!finish) return NULL;
	if ((len = PyList_Size(finish)) < 0) return NULL;
	for (i = 0; i < len; i++) {
	    PyObject *str = PyList_GET_ITEM(finish, i);
	    PyObject_CallFunctionObjArgs(write, str, NULL);
	}
    }
    /*for k, v in xmlns_items:*/
    PyObject *item, *v;
    if ((len = PyList_Size(xmlns_items)) < 0) return NULL;
    for (i = 0; i < len; i++) {
	/* del namespaces[v]*/
	item = PyList_GET_ITEM(xmlns_items, i);
	v = PyTuple_GET_ITEM(item, 1);
	PyDict_DelItem(namespaces, v);
    }

    /*node.write_html_tail(write, namespaces, xmlns_items)*/

    PyObject *tail = NULL;

    if (!(tail = PyObject_GetAttr(node, PySTRtail))) return NULL;
    if (tail != Py_None) {
	tail = PyObject_CallMethodObjArgs(node, PySTRget_html_tail, namespaces, 
					  xmlns_items, NULL);
	if ((len = PyList_Size(tail)) < 0) return NULL;
	for (i = 0; i < len; i++) {
	    PyObject *str = PyList_GET_ITEM(tail, i);
	    PyObject_CallFunctionObjArgs(write, str, NULL);
	}

    }

    return node;

}

static PyObject*
write_htmlhandler(PyObject *self, PyObject *args)
{
    PyObject *node, *write, *namespaces, *result;

    if (!PyArg_ParseTuple(args, "OOO:write_html", &write, &node, &namespaces)) {
	return NULL;
    }

    result = write_html(write, node, namespaces);
    if (!(result)) return NULL;
    return node;
}

static char write_htmlhandler_doc[] =
"write_html(write, node, namespaces)\n\
\n\
Serialize a node to HTML.\n";

static PyMethodDef methods[] = {
    {"clone", clonehandler, METH_VARARGS, clonehandler_doc},
    {"getiterator", getiteratorhandler, METH_VARARGS, getiteratorhandler_doc},
    {"findmeld", findmeldhandler, METH_VARARGS, findmeldhandler_doc},
    {"write_html", write_htmlhandler, METH_VARARGS, write_htmlhandler_doc},
    {NULL, NULL}
};

PyMODINIT_FUNC
initcmeld3(void) 
{
#define DEFINE_STRING(s) \
    if (!(PySTR##s = PyString_FromString(#s))) return 
    DEFINE_STRING(__class__); 
    DEFINE_STRING(__dict__); 
    DEFINE_STRING(_children);
    DEFINE_STRING(parent);
    DEFINE_STRING(tag);
    DEFINE_STRING(attrib);
    DEFINE_STRING(text);
    DEFINE_STRING(tail);
    DEFINE_STRING(structure);
    DEFINE_STRING(get_html_start);
    DEFINE_STRING(get_html_finish);
    DEFINE_STRING(get_html_tail);
    DEFINE_STRING(__dict__);
#undef DEFINE_STRING
    PySTR_MELD_ID = PyString_FromString(_MELD_ID);
    if (!PySTR_MELD_ID) {
	return;
    }
    Py_InitModule3("cmeld3", methods,
		   "C helpers for meld3");
}
