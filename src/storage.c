//******************************************************************************
//
// storage.c
//
// It would be nice to have a standards for storing and loading data from files.
// I looked at XML, but the language didn't really appeal to me. YAML
// (http://www.yaml.org) caught my eye, but I did not manage to find a set of
// YAML utilities for C, so I decided to try and design my own conventions for
// storing data. Most of them are based on what I learned from my (brief)
// exposure to YAML.
//
//******************************************************************************

#include "mud.h"
#include "utils.h"    // trim
#include "storage.h"



//*****************************************************************************
//
// local datastructures, functions, and variables
//
//*****************************************************************************
#define SET_MARKER            '-'
#define LIST_MARKER           '='
#define STRING_MARKER         '~'
#define TYPELESS_MARKER       ' '


struct storage_set {
  HASHTABLE *entries;
  int    longest_key;
  int      top_entry;
};

struct storage_set_list {
  LIST            *list;
  LIST_ITERATOR *list_i;
};

typedef struct storage_data {
  char                  *key; // what is the variable name we're known by?
  char              *str_val; // what is the data's string value?
  STORAGE_SET_LIST *list_val; // list value?
  STORAGE_SET       *set_val; // set value?
  int              entry_num; // used by storage_set to keep track of order
} STORAGE_DATA;


/* local functions */
void delete_storage_set (STORAGE_SET *set);
void delete_storage_list(STORAGE_SET_LIST *list);
void delete_storage_data(STORAGE_DATA *data);

bool list_is_empty(STORAGE_SET_LIST *list);
bool set_is_empty (STORAGE_SET *set);

void write_storage_set(STORAGE_SET *set, FILE *fl, int indent);
void write_storage_list(STORAGE_SET_LIST *list, FILE *fl, int indent);
void write_storage_data(STORAGE_DATA *data, FILE *fl, int key_width,int indent);


void delete_storage_set(STORAGE_SET *set) {
  HASH_ITERATOR *hash_i = newHashIterator(set->entries);
  STORAGE_DATA    *data = NULL;
  const char       *key = NULL;
  
  ITERATE_HASH(key, data, hash_i)
    delete_storage_data(data);
  deleteHashIterator(hash_i);

  deleteHashtable(set->entries);
  free(set);
}

void delete_storage_list(STORAGE_SET_LIST *list) {
  if(list->list_i) deleteListIterator(list->list_i);
  deleteListWith(list->list, delete_storage_set);
  free(list);
}

void delete_storage_data(STORAGE_DATA *data) {
  if(data->key)      free(data->key);
  if(data->str_val)  free(data->str_val);
  if(data->list_val) delete_storage_list(data->list_val);
  if(data->set_val)  delete_storage_set(data->set_val);
  free(data);
}

STORAGE_DATA *new_storage_data(const char *key) {
  STORAGE_DATA *data = malloc(sizeof(STORAGE_DATA));
  bzero(data, sizeof(*data));
  data->key     = strdup(key);
  return data;
}

STORAGE_DATA *new_data_set(STORAGE_SET *val, const char *key) {
  STORAGE_DATA *data = new_storage_data(key);
  data->set_val  = val;
  data->list_val = new_storage_list();
  data->str_val  = strdup("");
  return data;
}

STORAGE_DATA   *new_data_list(STORAGE_SET_LIST *val, const char *key) {
  STORAGE_DATA *data = new_storage_data(key);
  data->list_val = val;
  data->set_val  = new_storage_set();
  data->str_val  = strdup("");
  return data;
}

STORAGE_DATA *new_data_string(const char *val, const char *key) {
  STORAGE_DATA *data = new_storage_data(key);
  data->str_val      = strdup(val);
  data->list_val     = new_storage_list();
  data->set_val      = new_storage_set();
  return data;
}

STORAGE_DATA    *new_data_int(int val, const char *key) {
  char str_val[20]; sprintf(str_val, "%d", val);
  return new_data_string(str_val, key);
}

STORAGE_DATA    *new_data_long(long val, const char *key) {
  char str_val[20]; sprintf(str_val, "%ld", val);
  return new_data_string(str_val, key);
}

STORAGE_DATA *new_data_double(double val, const char *key) {
  char str_val[20]; sprintf(str_val, "%lf", val);
  return new_data_string(str_val, key);
}


void print_indent(FILE *fl, int indent) {
  if(indent > 0) {
    char fmt[20];
    sprintf(fmt, "%%%ds", indent);
    fprintf(fl, fmt, " ");
  }
}


//
// Print a key and the key delimeter to file
//
void print_key(FILE *fl, const char *key, int key_width, int indent) {
  char fmt[30];
  sprintf(fmt, "%%-%ds:", key_width);
  print_indent(fl, indent);
  fprintf(fl, fmt, key);
}


//
// write a string containing newlines to a file
//
void write_string_data(const char *string, FILE *fl, int indent) {
  static char buf[SMALL_BUFFER];
  int i, str_i;

  for(str_i = i = 0; string[str_i] != '\0'; str_i++) {
    buf[i] = string[str_i];
    if(buf[i] == '\n') {
      buf[++i] = '\0';
      print_indent(fl, indent);
      fprintf(fl, "%s", buf);
      i = 0;
    }
    else
      i++;
  }
}


//
// return true if the storage set is empty, and false if it is not
//
bool set_is_empty(STORAGE_SET *set) {
  // if the hashtable has a size of 0, we know for sure it's empty
  if(hashSize(set->entries) > 0) {
    HASH_ITERATOR *hash_i = newHashIterator(set->entries);
    STORAGE_DATA    *data = NULL;
    const char       *key = NULL;

    ITERATE_HASH(key, data, hash_i) {
      if(*data->str_val || 
	 !set_is_empty(data->set_val) || !list_is_empty(data->list_val)) {
	deleteHashIterator(hash_i);
	return FALSE;
      }
    }
    deleteHashIterator(hash_i);
  }

  return TRUE;
}


//
// return true if the storage list is empty, and false if it is not
//
bool list_is_empty(STORAGE_SET_LIST *list) {
  // if the list size is 0, we know for sure it's empty
  if(listSize(list->list) > 0) {
    LIST_ITERATOR *list_i = newListIterator(list->list);
    STORAGE_SET      *set = NULL;
    ITERATE_LIST(set, list_i) {
      if(!set_is_empty(set)) {
	deleteListIterator(list_i);
	return FALSE;
      }
    }
    deleteListIterator(list_i);
  }
  return TRUE;
}


void write_storage_data(STORAGE_DATA *data, FILE *fl, int key_width,int indent){
  // first, we see if we have a string value. If we do, print it
  if(*data->str_val) {
    print_key(fl, data->key, key_width, indent);
    // if we have a newline in our string, we have to write
    // it in a special way so as to preserve the lines
    if(count_letters(data->str_val, '\n', strlen(data->str_val)) > 0) {
      // first, print the string marker and skip down to a newline
      fprintf(fl, "%c\n", STRING_MARKER);
      // now, write the string
      write_string_data(data->str_val, fl, indent+2);
    }
    else
      fprintf(fl, "%c%s\n", TYPELESS_MARKER, data->str_val);
  }

  // If that fails, check if we have a set value. If we do, print it
  else if(!set_is_empty(data->set_val)) {
    print_key(fl, data->key, key_width, indent);
    fprintf(fl, "%c\n", SET_MARKER);
    write_storage_set(data->set_val, fl, indent+2);
  }

  // otherwise, check if we have a list value. If we do, print it
  else if(!list_is_empty(data->list_val)) {
    print_key(fl, data->key, key_width, indent);
    fprintf(fl, "%c\n", LIST_MARKER);
    write_storage_list(data->list_val, fl, indent+2);
  }
}


//
// Compares two variables on their entry_num. This is used by
// write_storage_set to print data out in the same order it was
// entered.
//
int cmp_storage_vars(STORAGE_DATA *data1, STORAGE_DATA *data2) {
  if(data1->entry_num == data2->entry_num)
    return 0;
  else if(data1->entry_num < data2->entry_num)
    return -1;
  return 1;
}


void write_storage_set(STORAGE_SET *set, FILE *fl, int indent) {
  // collect all of the items in our hashtable
  LIST           *elems = newList();
  HASH_ITERATOR *hash_i = newHashIterator(set->entries);
  STORAGE_DATA    *data = NULL;
  const char       *key = NULL;

  ITERATE_HASH(key, data, hash_i)
    listPut(elems, data);
  deleteHashIterator(hash_i);

  // now, sort them all
  listSortWith(elems, cmp_storage_vars);

  // now, for each one, print it
  while( (data = listPop(elems)) != NULL)
    write_storage_data(data, fl, set->longest_key, indent);
  deleteList(elems);

  // print our indent and the end-of-set marker
  print_indent(fl, indent);
  fprintf(fl, "%c\n", SET_MARKER);
}


void write_storage_list(STORAGE_SET_LIST *list, FILE *fl, int indent) {
  LIST_ITERATOR *list_i = newListIterator(list->list);
  STORAGE_SET      *set = NULL;

  ITERATE_LIST(set, list_i)
    write_storage_set(set, fl, indent);
  deleteListIterator(list_i);
}


//*****************************************************************************
//
// All of the functions required for parsing data out of files can be found
// here.
//
//*****************************************************************************

/* local functions */
STORAGE_SET      *parse_storage_set(FILE *fl, int indent);
STORAGE_SET_LIST *parse_storage_list(FILE *fl, int indent);


//
// skip ahead in our indent. If we can skip that far ahead,
// return TRUE. otherwise, return FALSE.
//
bool skip_indent(FILE *fl, int indent) {
  int i;
  char c;
  for(i = 0; i < indent; i++) {
    c = fgetc(fl);
    if(c != ' ')
      break;
  }

  if(i != indent) {
    fseek(fl, -(i+1), SEEK_CUR);
    return FALSE;
  }
  else
    return TRUE;
}


//
// Check to see if we're at the end of a storage entry. If we are,
// return true. otherwise, return false.
//
bool storage_end(FILE *fl) {
  char c = fgetc(fl);
  if(c == SET_MARKER) {
    // also skip the newline that comes after us
    fgetc(fl);
    return TRUE;
  }
  else {
    fseek(fl, -1, SEEK_CUR);
    return FALSE;
  }
}


//
// return the type of the data we're dealing with. It is assumed
// this will be called IMMEDIATELY after parse_key is called
//
char parse_type(FILE *fl) {
  return getc(fl);
}


//
// Parse the name of the key that is immediately in front of us
//
char *parse_key(FILE *fl) {
  static char buf[SMALL_BUFFER];
  char c;
  int  i = 0;
  // parse up to the colon, which is the marker for the end of the key
  while((c = fgetc(fl)) != EOF) {
    if(c == ':') {
      buf[i] = '\0';
      break;
    }
    buf[i++] = c;
  }
  // trim off all of the trailing spaces and return a copy
  trim(buf);
  return strdup(buf);
}


//
// Read until we hit a newline. Return a copy of what we find
//
char *parse_line(FILE *fl) {
  static char buf[SMALL_BUFFER];
  fgetline(fl, buf, SMALL_BUFFER);
  return strdup(buf);
}


//
// read in a string that may possibly have multiple newlines in it
//
char *parse_string(FILE *fl, int indent) {
  static char line[SMALL_BUFFER];
  static char buf [MAX_BUFFER];
  *buf = '\0';
  int i = 0;

  // as long as we can skip up our indent, we can read in data and
  // all is good. Once we can no longer skip up our indent, then
  // we have come to the end of our string
  while(skip_indent(fl, indent)) {
    fgetline(fl, line, SMALL_BUFFER);
    i += snprintf(buf+i, MAX_BUFFER-i-1, "%s\n", line);
  }

  return strdup(buf);
}


//
// Read in a list of storage sets. Return what we find.
//
STORAGE_SET_LIST *parse_storage_list(FILE *fl, int indent) {
  STORAGE_SET_LIST *list = new_storage_list();
  STORAGE_SET       *set = NULL;

  // read in each set in our list
  while( (set = parse_storage_set(fl, indent)) != NULL)
    storage_list_put(list, set);
  return list;
}


//
// Parse one storage set and return it
//
STORAGE_SET *parse_storage_set(FILE *fl, int indent) {
  STORAGE_SET *set = new_storage_set();
  int        loops = 0;

  while(skip_indent(fl, indent)) {
    loops++;
    if(storage_end(fl))
      break;

    char *key = parse_key(fl);
    char type = parse_type(fl);

    switch(type) {
    case TYPELESS_MARKER: {
      char *line = parse_line(fl);
      store_string(set, key, line);
      free(line);
      break;
    }

    case STRING_MARKER: {
      fgetc(fl); // kill the newline
      char *string = parse_string(fl, indent+2);
      store_string(set, key, string);
      free(string);
      break;
    }

    case SET_MARKER:
      fgetc(fl); // kill the newline
      store_set(set, key, parse_storage_set(fl, indent+2));
      break;

    case LIST_MARKER:
      fgetc(fl); // kill the newline
      store_list(set, key, parse_storage_list(fl, indent+2));
      break;
    }
    free(key);
  }

  if(loops == 0) {
    delete_storage_set(set);
    return NULL;
  }
  return set;
}




//*****************************************************************************
//
// implementation of storage.h
// documentation contained in storage.h
//
//*****************************************************************************
void storage_write(STORAGE_SET *set, const char *fname) {
  FILE *fl = NULL;
  // we wanted to open a file, but we couldn't ... abort
  if((fl = fopen(fname, "w+")) == NULL)
    return;
  write_storage_set(set, fl, 0);
  fclose(fl);
}


STORAGE_SET *new_storage_set() {
  STORAGE_SET *set = malloc(sizeof(STORAGE_SET));
  set->entries     = newHashtable(20);
  set->longest_key = 0;
  set->top_entry   = 0;
  return set;
}


void storage_close(STORAGE_SET *set) {
  delete_storage_set(set);
}


STORAGE_SET *storage_read(const char *fname) {
  FILE *fl = NULL;
  // we wanted to open a file, but we couldn't ... return an empty set
  if((fl = fopen(fname, "r")) == NULL)
    return NULL;//    return new_storage_set();
  STORAGE_SET *set = parse_storage_set(fl, 0);
  fclose(fl);
  return set;
}

STORAGE_SET_LIST *new_storage_list() {
  STORAGE_SET_LIST *list = malloc(sizeof(STORAGE_SET_LIST));
  list->list = newList();
  list->list_i = NULL;
  return list;
}

STORAGE_SET *storage_list_next(STORAGE_SET_LIST *list) {
  if(list->list_i == NULL)
    list->list_i = newListIterator(list->list);
  else
    listIteratorNext(list->list_i);
  return listIteratorCurrent(list->list_i);
}

void storage_list_put(STORAGE_SET_LIST *list, STORAGE_SET *set) {
  listQueue(list->list, set);
}

void   storage_put(STORAGE_SET *set, STORAGE_DATA *data) {
  // see if it's our longest key
  int key_len      = strlen(data->key);
  set->longest_key = MAX(set->longest_key, key_len);
  // if we already have data by this name, delete it
  STORAGE_DATA *olddata = hashGet(set->entries, data->key);
  if(olddata) delete_storage_data(olddata);
  // add the new data
  hashPut(set->entries, data->key, data);
  data->entry_num = ++set->top_entry;
}


void   store_set(STORAGE_SET *set, const char *key, STORAGE_SET *val) {
  storage_put(set, new_data_set(val, key));
}

void   store_list(STORAGE_SET *set, const char *key, STORAGE_SET_LIST *val) {
  storage_put(set, new_data_list(val, key));
}

void store_string(STORAGE_SET *set, const char *key, const char *val) {
  storage_put(set, new_data_string(val, key));
}

void store_double(STORAGE_SET *set, const char *key, double val) {
  storage_put(set, new_data_double(val, key));
}

void store_int(STORAGE_SET *set, const char *key, int val) {
  storage_put(set, new_data_int(val, key));
}

void store_long(STORAGE_SET *set, const char *key, long val) {
  storage_put(set, new_data_long(val, key));
}

STORAGE_SET    *read_set(STORAGE_SET *set, const char *key) {
  STORAGE_DATA *data = hashGet(set->entries, key);
  if(data) 
    return data->set_val;
  else {
    store_set(set, key, new_storage_set());
    return read_set(set, key);
  }
}

STORAGE_SET_LIST    *read_list(STORAGE_SET *set, const char *key) {
  STORAGE_DATA *data = hashGet(set->entries, key);
  if(data) 
    return data->list_val;
  else {
    store_list(set, key, new_storage_list());
    return read_list(set, key);
  }
}

const char *read_string(STORAGE_SET *set, const char *key) {
  STORAGE_DATA *data = hashGet(set->entries, key);
  if(data) return data->str_val;
  else     return "";
}

double read_double(STORAGE_SET *set, const char *key) {
  STORAGE_DATA *data = hashGet(set->entries, key);
  if(data) return atof(data->str_val);
  else     return 0;
}

int read_int(STORAGE_SET *set, const char *key) {
  STORAGE_DATA *data = hashGet(set->entries, key);
  if(data) return atoi(data->str_val);
  else     return 0;
}

long read_long(STORAGE_SET *set, const char *key) {
  STORAGE_DATA *data = hashGet(set->entries, key);
  if(data) return atol(data->str_val);
  else     return 0;
}

bool storage_contains(STORAGE_SET *set, const char *key) {
  return (hashGet(set->entries, key) != NULL);
}

//
// make a storage list out of a normal MUD list
//
STORAGE_SET_LIST *gen_store_list(LIST *list, void *storer) {
  STORAGE_SET *(* store_func)(void *) = storer;
  STORAGE_SET_LIST *set_list = new_storage_list();

  LIST_ITERATOR *list_i = newListIterator(list);
  void            *elem = NULL;

  ITERATE_LIST(elem, list_i)
    storage_list_put(set_list, store_func(elem));
  deleteListIterator(list_i);
  return set_list;
}


//
// parse a MUD list out of a storage list
//
LIST *gen_read_list(STORAGE_SET_LIST *list, void *reader) {
  void *(* read_func)(STORAGE_SET *) = reader;
  LIST *newlist = newList();

  STORAGE_SET *elem = NULL;
  while( (elem = storage_list_next(list)) != NULL)
    listPut(newlist, read_func(elem));

  return newlist;
}
