#ifndef PYPLUGS_H
#define PYPLUGS_H
//*****************************************************************************
//
// pyplugs.h
//
// Pyplugs are just various things that occur at the intersection between 
// Python and C, in NakedMud. They include the loading of python modules,
// a few commands for interacting with python modules, and miscellaneory (sp?)
// python stuff that doesn't neccessarily fall under "scripting".
//
//*****************************************************************************

// 
// Returns a string representation of the last error that Python threw. Must
// be freed afterwards.
char *getPythonTraceback(void);

//
// initialize all of our plugs with python
void init_pyplugs();

//
// Takes in a PyType, and adds lists of get/setters and methods to it. The
// lists can each be NULL if there are no getsetters or methods, respectively
void makePyType(PyTypeObject *type, LIST *getsetters, LIST *methods);

#endif // PYPLUGS_H