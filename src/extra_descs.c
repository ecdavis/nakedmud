//*****************************************************************************
//
// extra_desc.c
//
// Extra descriptions are little embellishments to rooms that give extra
// information about the setting if people examine them. Each edesc has a list
// of keywords that activates it. The extra_desc name is a bit misleading,
// as it also can be (and is) used to hold speech keywords/replies for NPCs.
//
//*****************************************************************************

#include "mud.h"
#include "utils.h"
#include "storage.h"
#include "extra_descs.h"

struct edesc_set_data {
  LIST *edescs;
};

struct edesc_data {
  EDESC_SET *set;
  char *keywords;
  char *desc;
};



//*****************************************************************************
//
// edesc set
//
//*****************************************************************************
EDESC_SET  *newEdescSet         () {
  EDESC_SET *set = malloc(sizeof(EDESC_SET));
  set->edescs = newList();
  return set;
}

EDESC_DATA *edescRead(STORAGE_SET *set) {
  return newEdesc(read_string(set, "keywords"), read_string(set, "desc"));
}

STORAGE_SET *edescStore(EDESC_DATA *data) {
  STORAGE_SET *set = new_storage_set();
  store_string(set, "keywords", data->keywords);
  store_string(set, "desc",     data->desc);
  return set;
}

EDESC_SET *edescSetRead(STORAGE_SET *set) {
  EDESC_SET      *edescs = newEdescSet();
  STORAGE_SET_LIST *list = read_list(set, "list");
  deleteList(edescs->edescs);
  edescs->edescs = gen_read_list(list, edescRead);
  LIST_ITERATOR *list_i = newListIterator(edescs->edescs);
  EDESC_DATA     *edesc = NULL;
  ITERATE_LIST(edesc, list_i)
    edesc->set = edescs;
  deleteListIterator(list_i);
  return edescs;
}


STORAGE_SET *edescSetStore(EDESC_SET *edescs) {
  STORAGE_SET *set             = new_storage_set();
  store_list(set, "list", gen_store_list(edescs->edescs, edescStore));
  return set;
}


void copyEdescSetTo(EDESC_SET *from, EDESC_SET *to) {
  // delete all of the current entries
  deleteListWith(to->edescs, deleteEdesc);
  to->edescs = newList();

  // copy all of them over and set their parent to us
  EDESC_DATA    *entry   = NULL;
  LIST_ITERATOR *edesc_i = newListIterator(from->edescs);
  ITERATE_LIST(entry, edesc_i)
    edescSetPut(to, copyEdesc(entry));
  deleteListIterator(edesc_i);
}


EDESC_SET *copyEdescSet(EDESC_SET *set) {
  EDESC_SET *newset = newEdescSet();
  copyEdescSetTo(set, newset);
  return newset;
}

EDESC_DATA *getEdesc            (EDESC_SET *set, const char *keyword) {
  LIST_ITERATOR *edesc_i = newListIterator(set->edescs);
  EDESC_DATA       *desc = NULL;

  ITERATE_LIST(desc, edesc_i)
    if(isEdescKeyword(desc, keyword))
      break;
  
  deleteListIterator(edesc_i);
  return desc;
}

void        edescSetPut         (EDESC_SET *set, EDESC_DATA *edesc) {
  edesc->set = set;
  listQueue(set->edescs, edesc);
}

EDESC_DATA *getEdescNum         (EDESC_SET *set, int num) {
  return listGet(set->edescs, num);
}

void removeEdesc         (EDESC_SET *set, EDESC_DATA *edesc) {
  if(listRemove(set->edescs, edesc))
    edesc->set = NULL;
}

EDESC_DATA *removeEdescEntry    (EDESC_SET *set, const char *keyword) {
  EDESC_DATA *entry = getEdesc(set, keyword);
  if(entry && listRemove(set->edescs, entry))
    entry->set = NULL;
  return entry;
}

EDESC_DATA *removeEdescEntryNum (EDESC_SET *set, int num) {
  EDESC_DATA *entry = listRemoveNum(set->edescs, num);
  if(entry)
    entry->set = NULL;
  return entry;
}

void        deleteEdescSet      (EDESC_SET *set) {
  EDESC_DATA *entry;

  while( (entry = listPop(set->edescs)) != NULL)
    deleteEdesc(entry);

  deleteList(set->edescs);
  free(set);
}

int         getEdescSetSize     (EDESC_SET *set) {
  return listSize(set->edescs);
}

LIST       *getEdescList        (EDESC_SET *set) {
  return set->edescs;
}


char       *tagEdescs           (EDESC_SET *set, const char *string,
				 const char *start_tag, const char *end_tag) {
  LIST_ITERATOR *list_i = newListIterator(set->edescs);
  EDESC_DATA    *edesc  = NULL;
  char          *newstring = strdup(string);

  // go through, and apply colors for each extra desc we have
  ITERATE_LIST(edesc, list_i) {
    char *oldstring = newstring;
    newstring = tag_keywords(edesc->keywords, oldstring, start_tag, end_tag);
    free(oldstring);
  }
  deleteListIterator(list_i);

  return newstring;
}



//*****************************************************************************
//
// a single edesc
//
//*****************************************************************************
EDESC_DATA *newEdesc(const char *keywords, const char *desc) {
  EDESC_DATA *edesc = malloc(sizeof(struct edesc_data));

  edesc->set      = NULL;
  edesc->desc     = (desc ? strdup(desc) : strdup(""));
  edesc->keywords = strdup((keywords ? keywords : ""));

  return edesc;
}

void deleteEdesc(EDESC_DATA *edesc) {
  if(edesc->keywords) free(edesc->keywords);
  if(edesc->desc)     free(edesc->desc);
  free(edesc);
}

EDESC_DATA *copyEdesc(EDESC_DATA *edesc) {
  return newEdesc(edesc->keywords, edesc->desc);
}

void copyEdescTo(EDESC_DATA *from, EDESC_DATA *to) {
  // copy over the new desc
  if(to->desc) free(to->desc);
  to->desc = strdup(from->desc ? from->desc : "");
  if(to->keywords) free(to->keywords);
  to->keywords = strdup(from->keywords ? from->keywords : "");
}

const char *getEdescKeywords(EDESC_DATA *edesc) {
  return edesc->keywords;
}

const char *getEdescDescription(EDESC_DATA *edesc) {
  return edesc->desc;
}

char **getEdescPtr(EDESC_DATA *edesc) {
  return &(edesc->desc);
}

void setEdescKeywords(EDESC_DATA *edesc, const char *keywords) {
  if(edesc->keywords) free(edesc->keywords);
  edesc->keywords   = strdup((keywords ? keywords :""));
}

void setEdescDescription(EDESC_DATA *edesc, const char *description) {
  if(edesc->desc) free(edesc->desc);
  edesc->desc = (description ? strdup(description) : strdup(""));
}

EDESC_SET *getEdescSet(EDESC_DATA *edesc) {
  return edesc->set;
}

bool isEdescKeyword(EDESC_DATA *edesc, const char *keyword) {
  return is_keyword(edesc->keywords, keyword, TRUE);
}
