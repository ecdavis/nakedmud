//*****************************************************************************
//
// exit.c
//
// exits are structures that keep information about the links between rooms.
//
//*****************************************************************************

#include "mud.h"
#include "utils.h"
#include "storage.h"
#include "exit.h"

#define EX_CLOSED            (1 << 0)
#define EX_LOCKED            (1 << 1)
// lockable is handled if the exit has a key


struct exit_data {
  char *name;              // what is the name of our door for descriptions?
  char *keywords;          // what keywords can the door be referenced by?
  char *description;       // what does a person see when they look at us?

  char *spec_enter;        // the message when we enter from this exit
  char *spec_leave;        // the message when we leave through this exit

  bitvector_t status;      // closable, closed, locked, etc...

  int closable;            // is the exit closable?
  obj_vnum key;            // what is the vnum of the key?
  int hide_lev;            // how hidden is this exit?
  int pick_lev;            // how hard is it to pick this exit?

  room_vnum to;            // where do we exit to?
};



EXIT_DATA *newExit() {
  EXIT_DATA *exit = malloc(sizeof(EXIT_DATA));
  exit->name        = strdup("");
  exit->keywords    = strdup("");
  exit->description = strdup("");
  exit->spec_enter  = strdup("");
  exit->spec_leave  = strdup("");
  exit->key    = NOTHING;
  exit->to     = NOWHERE;
  exit->hide_lev = 0;
  exit->pick_lev = 0;
  exit->status   = 0;
  exit->closable = FALSE;
  return exit;
};


void deleteExit(EXIT_DATA *exit) {
  if(exit->name)        free(exit->name);
  if(exit->description) free(exit->description);
  if(exit->spec_enter)  free(exit->spec_enter);
  if(exit->spec_leave)  free(exit->spec_leave);
  if(exit->keywords)    free(exit->keywords);

  free(exit);
};


void exitCopyTo(const EXIT_DATA *from, EXIT_DATA *to) {
  exitSetName       (to, exitGetName(from));
  exitSetKeywords   (to, exitGetKeywords(from));
  exitSetDescription(to, exitGetDesc(from));
  exitSetTo         (to, exitGetTo(from));
  exitSetPickLev    (to, exitGetPickLev(from));
  exitSetHidden     (to, exitGetHidden(from));
  exitSetKey        (to, exitGetKey(from));
  exitSetLocked     (to, exitIsLocked(from));
  exitSetClosed     (to, exitIsClosed(from));
  exitSetClosable   (to, exitIsClosable(from));
  exitSetSpecEnter  (to, exitGetSpecEnter(from));
  exitSetSpecLeave  (to, exitGetSpecLeave(from));
}


EXIT_DATA *exitCopy(const EXIT_DATA *exit) {
  EXIT_DATA *E = newExit();
  exitCopyTo(exit, E);
  return E;
}

EXIT_DATA *exitRead(STORAGE_SET *set) {
  EXIT_DATA *exit = newExit();
  exitSetName(exit,        read_string(set, "name"));
  exitSetKeywords(exit,    read_string(set, "keywords"));
  exitSetDescription(exit, read_string(set, "desc"));
  exitSetSpecEnter(exit,   read_string(set, "enter"));
  exitSetSpecLeave(exit,   read_string(set, "leave"));
  exitSetTo(exit,          read_int   (set, "to"));
  exitSetKey(exit,         read_int   (set, "key"));
  exitSetHidden(exit,      read_int   (set, "hide_level"));
  exitSetPickLev(exit,     read_int   (set, "pick_level"));
  exitSetClosable(exit,    read_int   (set, "closable"));
  return exit;
}

STORAGE_SET *exitStore(EXIT_DATA *exit) {
  STORAGE_SET *set = new_storage_set();
  store_string(set, "name",       exit->name,       NULL);
  store_string(set, "keywords",   exit->keywords,   NULL);
  store_string(set, "desc",       exit->description,NULL);
  store_string(set, "enter",      exit->spec_enter, NULL);
  store_string(set, "leave",      exit->spec_leave, NULL);
  store_int   (set, "to",         exit->to,         NULL);
  store_int   (set, "key",        exit->key,        NULL);
  store_int   (set, "hide_level", exit->hide_lev,   NULL);
  store_int   (set, "pick_level", exit->pick_lev,   NULL);
  store_int   (set, "closable",   exit->closable,   NULL);
  return set;
}




//*****************************************************************************
//
// is, get and set functions
//
//*****************************************************************************

bool        exitIsClosable(const EXIT_DATA *exit) {
  return exit->closable;
};

bool        exitIsClosed(const EXIT_DATA *exit) {
  return IS_SET(exit->status, EX_CLOSED);
};

bool        exitIsLocked(const EXIT_DATA *exit) {
  return IS_SET(exit->status, EX_LOCKED);
};

bool        exitIsName(const EXIT_DATA *exit, const char *name) {
  return is_keyword(exit->keywords, name, TRUE);
}

int         exitGetHidden(const EXIT_DATA *exit) {
  return exit->hide_lev;
};

int         exitGetPickLev(const EXIT_DATA *exit) {
  return exit->pick_lev;
};

obj_vnum    exitGetKey(const EXIT_DATA *exit) {
  return exit->key;
};

room_vnum   exitGetTo(const EXIT_DATA *exit) {
  return exit->to;
};

const char *exitGetName(const EXIT_DATA *exit) {
  return exit->name;
};

const char *exitGetKeywords(const EXIT_DATA *exit) {
  return exit->keywords;
};

const char *exitGetDesc(const EXIT_DATA *exit) {
  return exit->description;
};

const char *exitGetSpecEnter(const EXIT_DATA *exit) {
  return exit->spec_enter;
};

const char *exitGetSpecLeave(const EXIT_DATA *exit) {
  return exit->spec_leave;
};

char      **exitGetDescPtr(EXIT_DATA *exit) {
  return &(exit->description);
};

void        exitSetClosable(EXIT_DATA *exit, bool closable) {
  exit->closable = (closable != 0);
};

void        exitSetClosed(EXIT_DATA *exit, bool closed) {
  if(closed)    SET_BIT(exit->status, EX_CLOSED);
  else          REMOVE_BIT(exit->status, EX_CLOSED);
};

void        exitSetLocked(EXIT_DATA *exit, bool locked) {
  if(locked)    SET_BIT(exit->status, EX_LOCKED);
  else          REMOVE_BIT(exit->status, EX_LOCKED);
};

void        exitSetKey(EXIT_DATA *exit, obj_vnum key) {
  exit->key = key;
};

void        exitSetHidden(EXIT_DATA *exit, int hide_lev) {
  exit->hide_lev = hide_lev;
};

void        exitSetPickLev(EXIT_DATA *exit, int pick_lev) {
  exit->pick_lev = pick_lev;
};

void        exitSetTo(EXIT_DATA *exit, room_vnum room) {
  exit->to = room;
};

void        exitSetName(EXIT_DATA *exit, const char *name) {
  if(exit->name) free(exit->name);
  if(name)       exit->name = strdup(name);
  else           exit->name = strdup("");
};

void        exitSetKeywords(EXIT_DATA *exit, const char *keywords) {
  if(exit->keywords) free(exit->keywords);
  if(keywords)       exit->keywords = strdup(keywords);
  else               exit->keywords = strdup("");
};

void        exitSetDescription(EXIT_DATA *exit, const char *desc) {
  if(exit->description) free(exit->description);
  if(desc)              exit->description = strdup(desc);
  else                  exit->description = strdup("");
};

void        exitSetSpecEnter(EXIT_DATA *exit, const char *enter) {
  if(exit->spec_enter)  free(exit->spec_enter);
  if(enter)             exit->spec_enter = strdup(enter);
  else                  exit->spec_enter = strdup("");
};

void        exitSetSpecLeave(EXIT_DATA *exit, const char *leave) {
  if(exit->spec_leave)  free(exit->spec_leave);
  if(leave)             exit->spec_leave = strdup(leave);
  else                  exit->spec_leave = strdup("");
};