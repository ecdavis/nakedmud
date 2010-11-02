//*****************************************************************************
//
// pyexit.c
//
// Contains a python exit module, and an Exit class that is a python wrapper
// for NakedMud exits.
//
//*****************************************************************************

#include <Python.h>
#include <structmember.h>

#include "../mud.h"
#include "../utils.h"
#include "../object.h"
#include "../room.h"
#include "../exit.h"

#include "pyroom.h"
#include "pyobj.h"
#include "pyplugs.h"
#include "pyexit.h"



//*****************************************************************************
// local structures and defines
//*****************************************************************************

// a list of the get/setters on the Exit class
LIST *pyexit_getsetters = NULL;

// a list of the methods on the Exit class
LIST *pyexit_methods = NULL;

typedef struct {
  PyObject_HEAD
  int uid;
} PyExit;



//*****************************************************************************
// allocation, deallocation, initialization, and comparison
//*****************************************************************************
void PyExit_dealloc(PyExit *self) {
  self->ob_type->tp_free((PyObject*)self);
}

PyObject *PyExit_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyExit *self;
    self = (PyExit *)type->tp_alloc(type, 0);
    self->uid = NOTHING;
    return (PyObject *)self;
}

int PyExit_init(PyExit *self, PyObject *args, PyObject *kwds) {
  char *kwlist[] = {"uid", NULL};
  int        uid = NOTHING;

  // get the uid
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &uid)) {
    PyErr_Format(PyExc_TypeError, "Exits may only be created using a uid");
    return -1;
  }

  // make sure a exit with the uid exists
  if(!propertyTableGet(exit_table, uid)) {
    PyErr_Format(PyExc_TypeError, 
		 "Exit with uid, %d, does not exist", uid);
    return -1;
  }

  self->uid = uid;
  return 0;
}

int PyExit_compare(PyExit *exit1, PyExit *exit2) {
  if(exit1->uid == exit2->uid)
    return 0;
  else if(exit1->uid < exit2->uid)
    return -1;
  else
    return 1;
}



//*****************************************************************************
// getters and setters for the Exit class
//*****************************************************************************
PyObject *PyExit_getuid(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("i", exitGetUID(ex));
  else           return NULL;  
}

PyObject *PyExit_getkey(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("s", exitGetKey(ex));
  else           return NULL;  
}

PyObject *PyExit_getdest(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("s", exitGetTo(ex));
  else           return NULL;  
}

PyObject *PyExit_getspotdiff(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("i", exitGetHidden(ex));
  else           return NULL;  
}

PyObject *PyExit_getpickdiff(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("i", exitGetPickLev(ex));
  else           return NULL;  
}

PyObject *PyExit_getkeywords(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("s", exitGetKeywords(ex));
  else           return NULL;  
}

PyObject *PyExit_getopposite(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("s", exitGetOpposite(ex));
  else           return NULL;  
}

PyObject *PyExit_getleavemssg(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("s", exitGetSpecLeave(ex));
  else           return NULL;  
}

PyObject *PyExit_getentermssg(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("s", exitGetSpecEnter(ex));
  else           return NULL;  
}

PyObject *PyExit_getdesc(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("s", exitGetDesc(ex));
  else           return NULL;  
}

PyObject *PyExit_getname(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("s", exitGetName(ex));
  else           return NULL;  
}

PyObject *PyExit_getclosable(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("b", exitIsClosable(ex));
  else           return NULL;  
}

PyObject *PyExit_getclosed(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("b", exitIsClosed(ex));
  else           return NULL;  
}

PyObject *PyExit_getlocked(PyObject *self, void *closure) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex != NULL) return Py_BuildValue("b", exitIsLocked(ex));
  else           return NULL;  
}


//
// Standard check to make sure the exit exists when trying to set a value for 
// it. If successful, assign the exit to ex. Otherwise, return -1 (error)
#define PYEXIT_CHECK_EXIT_EXISTS(uid, ex)                                      \
  ex = propertyTableGet(exit_table, uid);                                      \
  if(ex == NULL) {                                                             \
    PyErr_Format(PyExc_TypeError,                                              \
		    "Tried to modify nonexistent exit, %d", uid);              \
    return -1;                                                                 \
  }                                                                            

int PyExit_setkey(PyObject *self, PyObject *value, void *closure) {
  EXIT_DATA *ex = NULL;
  PYEXIT_CHECK_EXIT_EXISTS(PyExit_AsUid(self), ex);

  if(value == NULL || value == Py_None)
    exitSetKey(ex, "");
  else if(PyString_Check(value))
    exitSetKey(ex, PyString_AsString(value));
  else if(PyObj_Check(value)) {
    OBJ_DATA *obj = PyObj_AsObj(value);
    if(obj != NULL)
      exitSetKey(ex, objGetClass(obj));
    else {
      PyErr_Format(PyExc_TypeError,
		   "Tried to change exit %d's key to nonexistant object, %d.",
		   exitGetUID(ex), PyObj_AsUid(value));
      return -1;
    }
  }
  else {
    PyErr_Format(PyExc_TypeError,
		 "Tried to change exit %d's key to an invalid type.",
		 exitGetUID(ex));
    return -1;
  }

  return 0;
}


int PyExit_setdest(PyObject *self, PyObject *value, void *closure) {
  EXIT_DATA *ex = NULL;
  PYEXIT_CHECK_EXIT_EXISTS(PyExit_AsUid(self), ex);

  if(value == NULL || value == Py_None) {
    PyErr_Format(PyExc_StandardError, "Cannot delete an exit %d's destination. "
		 "delete the exit from its room instead.", exitGetUID(ex));
    return -1;
  }
  else if(PyString_Check(value))
    exitSetTo(ex, PyString_AsString(value));
  else if(PyRoom_Check(value)) {
    ROOM_DATA *room = PyRoom_AsRoom(value);
    if(room != NULL)
      exitSetTo(ex, roomGetClass(room));
    else {
      PyErr_Format(PyExc_StandardError, "Tried to set exit %d's destination to "
		   "nonexistant room, %d.", exitGetUID(ex),PyRoom_AsUid(value));
      return -1;
    }
  }
  else {
    PyErr_Format(PyExc_TypeError,
		 "Tried to change exit %d's destination to an invalid type.",
		 exitGetUID(ex));
    return -1;
  }

  return 0;
}

int PyExit_setspotdiff(PyObject *self, PyObject *value, void *closure) {
  EXIT_DATA *ex = NULL;
  PYEXIT_CHECK_EXIT_EXISTS(PyExit_AsUid(self), ex);

  if(value == NULL || value == Py_None)
    exitSetHidden(ex, 0);
  else if(PyInt_Check(value))
    exitSetHidden(ex, PyInt_AsLong(value));
  else {
    PyErr_Format(PyExc_TypeError,
		"Tried to change exit %d's spot difficulty to an invalid type.",
		 exitGetUID(ex));
    return -1;
  }

  return 0;
}

int PyExit_setpickdiff(PyObject *self, PyObject *value, void *closure) {
  EXIT_DATA *ex = NULL;
  PYEXIT_CHECK_EXIT_EXISTS(PyExit_AsUid(self), ex);

  if(value == NULL || value == Py_None)
    exitSetPickLev(ex, 0);
  else if(PyInt_Check(value))
    exitSetPickLev(ex, PyInt_AsLong(value));
  else {
    PyErr_Format(PyExc_TypeError,
		"Tried to change exit %d's pick difficulty to an invalid type.",
		 exitGetUID(ex));
    return -1;
  }
  return 0;
}

int PyExit_setkeywords(PyObject *self, PyObject *value, void *closure) {
  EXIT_DATA *ex = NULL;
  PYEXIT_CHECK_EXIT_EXISTS(PyExit_AsUid(self), ex);

  if(value == NULL || value == Py_None)
    exitSetKeywords(ex, "");
  else if(PyString_Check(value))
    exitSetKeywords(ex, PyString_AsString(value));
  else {
    PyErr_Format(PyExc_TypeError,
		"Tried to change exit %d's keywords to an invalid type.",
		 exitGetUID(ex));
    return -1;
  }

  return 0;
}

int PyExit_setopposite(PyObject *self, PyObject *value, void *closure) {
  EXIT_DATA *ex = NULL;
  PYEXIT_CHECK_EXIT_EXISTS(PyExit_AsUid(self), ex);

  if(value == NULL || value == Py_None)
    exitSetOpposite(ex, "");
  else if(PyString_Check(value))
    exitSetOpposite(ex, PyString_AsString(value));
  else {
    PyErr_Format(PyExc_TypeError,
		"Tried to change exit %d's opposite dir to an invalid type.",
		 exitGetUID(ex));
    return -1;
  }

  return 0;
}

int PyExit_setleavemssg(PyObject *self, PyObject *value, void *closure) {
  EXIT_DATA *ex = NULL;
  PYEXIT_CHECK_EXIT_EXISTS(PyExit_AsUid(self), ex);

  if(value == NULL || value == Py_None)
    exitSetSpecLeave(ex, "");
  else if(PyString_Check(value))
    exitSetSpecLeave(ex, PyString_AsString(value));
  else {
    PyErr_Format(PyExc_TypeError,
		"Tried to change exit %d's leave message to an invalid type.",
		 exitGetUID(ex));
    return -1;
  }

  return 0;
}

int PyExit_setentermssg(PyObject *self, PyObject *value, void *closure) {
  EXIT_DATA *ex = NULL;
  PYEXIT_CHECK_EXIT_EXISTS(PyExit_AsUid(self), ex);

  if(value == NULL || value == Py_None)
    exitSetSpecEnter(ex, "");
  else if(PyString_Check(value))
    exitSetSpecEnter(ex, PyString_AsString(value));
  else {
    PyErr_Format(PyExc_TypeError,
		"Tried to change exit %d's enter message to an invalid type.",
		 exitGetUID(ex));
    return -1;
  }

  return 0;
}

int PyExit_setdesc(PyObject *self, PyObject *value, void *closure) {
  EXIT_DATA *ex = NULL;
  PYEXIT_CHECK_EXIT_EXISTS(PyExit_AsUid(self), ex);

  if(value == NULL || value == Py_None)
    exitSetDesc(ex, "");
  else if(PyString_Check(value))
    exitSetDesc(ex, PyString_AsString(value));
  else {
    PyErr_Format(PyExc_TypeError,
		"Tried to change exit %d's desc to an invalid type.",
		 exitGetUID(ex));
    return -1;
  }

  return 0;
}

int PyExit_setname(PyObject *self, PyObject *value, void *closure) {
  EXIT_DATA *ex = NULL;
  PYEXIT_CHECK_EXIT_EXISTS(PyExit_AsUid(self), ex);

  if(value == NULL || value == Py_None)
    exitSetName(ex, "");
  else if(PyString_Check(value))
    exitSetName(ex, PyString_AsString(value));
  else {
    PyErr_Format(PyExc_TypeError,
		"Tried to change exit %d's door name to an invalid type.",
		 exitGetUID(ex));
    return -1;
  }

  return 0;
}



//*****************************************************************************
// methods for the Exit class
//*****************************************************************************
PyObject *PyExit_makedoor(PyExit *self, PyObject *value) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  char *name = NULL, *kwds = NULL, *opp = NULL;
  bool closed = FALSE, locked = FALSE;
  PyObject *py_key = NULL;
  const char  *key = NULL;
  if(ex == NULL) {
    PyErr_Format(PyExc_StandardError, "Tried to edit nonexistant exit, %d.",
		 self->uid);
    return NULL;
  }

  if(!PyArg_ParseTuple(value, "|sssbbO", &name, &kwds, &opp, &closed, &locked,
		       &py_key)) {
    PyErr_Format(PyExc_TypeError, "Invalid types supplied to exit.makedoor()");
    return NULL;
  }

  // do we have a py_key? If so, figure out if it's an obj or a string
  if(py_key != NULL) {
    if(PyString_Check(py_key))
      key = PyString_AsString(py_key);
    else if(PyObj_Check(py_key)) {
      OBJ_DATA *obj = PyObj_AsObj(py_key);
      if(obj != NULL) key = objGetClass(obj);
    }

    // did we find a key ok?
    if(key == NULL) {
      PyErr_Format(PyExc_TypeError, "Supplied invalid key type for exit, %d.",
		   exitGetUID(ex));
      return NULL;
    }
  }

  // do all the setting of our values...
  if(name != NULL)
    exitSetName(ex, name);
  if(kwds != NULL)
    exitSetKeywords(ex, kwds);

  // if we pulled up an opposite, we know we probably also got closed and locked
  // status. Not always, but close enough...
  if(opp != NULL) {
    exitSetOpposite(ex, opp);
    exitSetClosed(ex, closed);
    exitSetLocked(ex, locked);
  }
  
  if(key != NULL)
    exitSetKey(ex, key);

  // success!
  return Py_BuildValue("i", 1);
}


PyObject *PyExit_open(PyExit *self, PyObject *value) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex == NULL) {
    PyErr_Format(PyExc_StandardError, 
		 "Tried to open door on nonexistent exit, %d.", self->uid);
    return NULL;
  }

  exitSetClosed(ex, FALSE);
  exitSetLocked(ex, FALSE);
  return Py_BuildValue("i", 1);
}


PyObject *PyExit_close(PyExit *self, PyObject *value) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex == NULL) {
    PyErr_Format(PyExc_StandardError, 
		 "Tried to close door on nonexistent exit, %d.", self->uid);
    return NULL;
  }

  exitSetClosed(ex, TRUE);
  return Py_BuildValue("i", 1);
}

PyObject *PyExit_lock(PyExit *self, PyObject *value) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex == NULL) {
    PyErr_Format(PyExc_StandardError, 
		 "Tried to lock door on nonexistent exit, %d.", self->uid);
    return NULL;
  }

  exitSetClosed(ex, TRUE);
  exitSetLocked(ex, TRUE);
  return Py_BuildValue("i", 1);
}

PyObject *PyExit_unlock(PyExit *self, PyObject *value) {
  EXIT_DATA *ex = PyExit_AsExit((PyObject *)self);
  if(ex == NULL) {
    PyErr_Format(PyExc_StandardError, 
		 "Tried to unlock door on nonexistent exit, %d.", self->uid);
    return NULL;
  }

  exitSetLocked(ex, FALSE);
  return Py_BuildValue("i", 1);
}



//*****************************************************************************
// structures to define our methods and classes
//*****************************************************************************
PyTypeObject PyExit_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "exit.Exit",               /*tp_name*/
    sizeof(PyExit),            /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PyExit_dealloc,/*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    (cmpfunc)PyExit_compare,   /*tp_compare*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Python Exit object",      /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,                         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */ 
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PyExit_init,     /* tp_init */
    0,                         /* tp_alloc */
    PyExit_new,                /* tp_new */
};

PyMethodDef exit_module_methods[] = {
  {NULL, NULL, 0, NULL}  /* Sentinel */
};



//*****************************************************************************
// implementation of pyexit.h
//*****************************************************************************
void PyExit_addGetSetter(const char *name, void *g, void *s, const char *doc) {
  // make sure our list of get/setters is created
  if(pyexit_getsetters == NULL) pyexit_getsetters = newList();

  // make the GetSetter def
  PyGetSetDef *def = calloc(1, sizeof(PyGetSetDef));
  def->name        = strdup(name);
  def->get         = (getter)g;
  def->set         = (setter)s;
  def->doc         = (doc ? strdup(doc) : NULL);
  def->closure     = NULL;
  listPut(pyexit_getsetters, def);
}

void PyExit_addMethod(const char *name, void *f, int flags, const char *doc) {
  // make sure our list of methods is created
  if(pyexit_methods == NULL) pyexit_methods = newList();

  // make the Method def
  PyMethodDef *def = calloc(1, sizeof(PyMethodDef));
  def->ml_name     = strdup(name);
  def->ml_meth     = (PyCFunction)f;
  def->ml_flags    = flags;
  def->ml_doc      = (doc ? strdup(doc) : NULL);
  listPut(pyexit_methods, def);
}

// initialize exits for use. This must be called AFTER 
PyMODINIT_FUNC
init_PyExit(void) {
    PyObject* module = NULL;

    // add all of the basic getsetters
    PyExit_addGetSetter("uid", PyExit_getuid, NULL,
			"returns the exit's universal ID nubmer");
    PyExit_addGetSetter("spot_diff", PyExit_getspotdiff, PyExit_setspotdiff,
			"integer value representing how hidden the exit is.");
    PyExit_addGetSetter("pick_diff", PyExit_getpickdiff, PyExit_setpickdiff,
			"integer value representing how hard lock is to pick.");
    PyExit_addGetSetter("key", PyExit_getkey, PyExit_setkey,
			"String or Obj value for obj class that unlocks exit.");
    PyExit_addGetSetter("dest", PyExit_getdest, PyExit_setdest,
			"String or Room value for room this exit leads to.");
    PyExit_addGetSetter("name", PyExit_getname, PyExit_setname,
			"The name of the door on the exit.");
    PyExit_addGetSetter("keywords", PyExit_getkeywords, PyExit_setkeywords,
			"comma-separated string of the door's keywords.");
    PyExit_addGetSetter("opposite", PyExit_getopposite, PyExit_setopposite,
			"if the exit is special, a dir name for the exit that "
			"leads back to this exit's room.");
    PyExit_addGetSetter("desc", PyExit_getdesc, PyExit_setdesc,
			"the long description of the exit when looked at.");
    PyExit_addGetSetter("leave_mssg", PyExit_getleavemssg, PyExit_setleavemssg,
			"the special message sent when a char exits the room.");
    PyExit_addGetSetter("enter_mssg", PyExit_getentermssg, PyExit_setentermssg, 
		       "the special message sent when a char enters the room.");
    PyExit_addGetSetter("is_closable", PyExit_getclosable, NULL,
			"true or false if the exit can be closed.");
    PyExit_addGetSetter("is_closed", PyExit_getclosed, NULL,
			"true or false if the exit is closed.");
    PyExit_addGetSetter("is_locked", PyExit_getlocked, NULL,
			"true or false if the exit is locked.");

    // add all of the basic methods
    PyExit_addMethod("makedoor", PyExit_makedoor, METH_VARARGS,
		     "Make a door on the exit. Takes name, keywords, and "
		     "optionally opposite, closed, locked, and key.");
    PyExit_addMethod("open", PyExit_open, METH_VARARGS,
		     "Opens the exit if there's a door. Also unlocks.");
    PyExit_addMethod("close", PyExit_close, METH_VARARGS,
		     "Close the exit's door if one exists.");
    PyExit_addMethod("lock", PyExit_lock, METH_VARARGS,
		     "Locks the exit if there's a door. Also closes.");
    PyExit_addMethod("unlock", PyExit_unlock, METH_VARARGS,
		     "Unlocks the exit's door if one exists.");

    // add in all the getsetters and methods
    makePyType(&PyExit_Type, pyexit_getsetters, pyexit_methods);
    deleteListWith(pyexit_getsetters, free); pyexit_getsetters = NULL;
    deleteListWith(pyexit_methods,    free); pyexit_methods    = NULL;

    // make sure the exit class is ready to be made
    if (PyType_Ready(&PyExit_Type) < 0)
        return;

    // initialize the module
    module = Py_InitModule3("exit", exit_module_methods,
			    "The exit module, for all MUD exit-related stuff.");

    // make sure the module parsed OK
    if (module == NULL)
      return;

    // add the Exit class to the exit module
    PyTypeObject *type = &PyExit_Type;
    PyModule_AddObject(module, "Exit", (PyObject *)type);
    Py_INCREF(&PyExit_Type);
}

int PyExit_AsUid(PyObject *exit) {
  return ((PyExit *)exit)->uid;
}

EXIT_DATA *PyExit_AsExit(PyObject *exit) {
  return propertyTableGet(exit_table, PyExit_AsUid(exit));
}

int PyExit_Check(PyObject *value) {
  return PyObject_TypeCheck(value, &PyExit_Type);
}

PyObject *
newPyExit(EXIT_DATA *exit) {
  PyExit *py_exit = (PyExit *)PyExit_new(&PyExit_Type, NULL, NULL);
  py_exit->uid = exitGetUID(exit);
  return (PyObject *)py_exit;
}