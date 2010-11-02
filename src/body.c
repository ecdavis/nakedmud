//*****************************************************************************
//
// body.c
//
// Different creatures are shaped in fundamentally different ways (e.g.
// bipedal humans and quadrapedal bears). Here's our attempt to create a
// structure that captures this idea.
//
//*****************************************************************************
#include "mud.h"
#include "body.h"
#include "utils.h"


struct bodypart_data {
  char          *name;       // the name of the position
  int            type;       // what kind of position type is this?
  int            size;       // how big is it, relative to other positions?
  OBJ_DATA *equipment;       // what is being worn here?
};

typedef struct bodypart_data   BODYPART;

struct body_data {
  LIST *parts;               // a list of all the parts on the body
};


//*****************************************************************************
//
// Local functions. Mosty concerned with the handling of bodypart_datas, as
// opposed to their container, the body.
//
//*****************************************************************************
const char *bodypos_list[NUM_BODYPOS] = {
  "floating about head",
  "head",
  "face",
  "ear",
  "neck",
  "about body",
  "torso",
  "arm",
  "wing",
  "wrist",
  "left hand",
  "right hand",
  "finger",
  "waist",
  "leg",
  "left foot",
  "right foot",
  "hoof",
  "claw",
  "tail",
  "held"
};

const char *bodytype_list[NUM_BODYTYPES] = {
  "humanoid",
  "dragon",
};

/**
 * Create a new bodypart_data
 */
BODYPART *newBodypart(const char *name, int type, int size) {
  BODYPART *P = malloc(sizeof(BODYPART));
  P->equipment = NULL;
  P->type = type;
  P->size = MAX(0, size); // parts of size 0 cannot be hit
  P->name = strdup((name ? name : "nothing"));

  return P;
};


/**
 * Delete a bodypart_data, but do not delete the equipment on it
 */
void deleteBodypart(BODYPART *P) {
  if(P->name) free(P->name);
  free(P);
};

/**
 * Copy a bodypart
 */
BODYPART *bodypartCopy(BODYPART *P) {
  BODYPART *p_new = malloc(sizeof(BODYPART));
  p_new->equipment = NULL;
  p_new->type      = P->type;
  p_new->size      = P->size;
  p_new->name      = strdup(P->name);
  return p_new;
}

/**
 * Create a new body with dragon pieces and proportions
 */
BODY_DATA *newDragonBody() {
  BODY_DATA *body = newBody();
  bodyAddPosition(body, "left hind claw",          BODYPOS_CLAW,          1);
  bodyAddPosition(body, "right hind claw",         BODYPOS_CLAW,          1);
  bodyAddPosition(body, "left front claw",         BODYPOS_CLAW,          1);
  bodyAddPosition(body, "right front claw",        BODYPOS_CLAW,          1);
  bodyAddPosition(body, "right hind leg",          BODYPOS_LEG,           8);
  bodyAddPosition(body, "left hind leg",           BODYPOS_LEG,           8);
  bodyAddPosition(body, "right front leg",         BODYPOS_LEG,           8);
  bodyAddPosition(body, "left front leg",          BODYPOS_LEG,           8);
  bodyAddPosition(body, "left wing",               BODYPOS_WING,         10);
  bodyAddPosition(body, "right wing",              BODYPOS_WING,         10);
  bodyAddPosition(body, "torso",                   BODYPOS_TORSO,        30);
  bodyAddPosition(body, "neck",                    BODYPOS_NECK,          3);
  bodyAddPosition(body, "face",                    BODYPOS_FACE,          3);
  bodyAddPosition(body, "head",                    BODYPOS_HEAD,          2);
  bodyAddPosition(body, "tail",                    BODYPOS_TAIL,          6);
  bodyAddPosition(body, "floating about head",     BODYPOS_FLOAT,         0);
  //                                                                  ------
  //                                                                    100
  return body;
}

/**
 * Create a new body with humanoid pieces and proportions
 */
BODY_DATA *newHumanoidBody() {
  BODY_DATA *body = newBody();
  bodyAddPosition(body, "right grip",              BODYPOS_HELD,          0);
  bodyAddPosition(body, "left grip",               BODYPOS_HELD,          0);
  bodyAddPosition(body, "right foot",              BODYPOS_RIGHT_FOOT,    2);
  bodyAddPosition(body, "left foot",               BODYPOS_LEFT_FOOT,     2);
  bodyAddPosition(body, "right leg",               BODYPOS_LEG,           9);
  bodyAddPosition(body, "left leg",                BODYPOS_LEG,           9);
  bodyAddPosition(body, "waist",                   BODYPOS_WAIST,         1);
  bodyAddPosition(body, "right finger",            BODYPOS_FINGER,        1);
  bodyAddPosition(body, "left finger",             BODYPOS_FINGER,        1);
  bodyAddPosition(body, "right hand",              BODYPOS_RIGHT_HAND,    2);
  bodyAddPosition(body, "left hand",               BODYPOS_LEFT_HAND,     2);
  bodyAddPosition(body, "right wrist",             BODYPOS_WRIST,         1);
  bodyAddPosition(body, "left wrist",              BODYPOS_WRIST,         1);
  bodyAddPosition(body, "right arm",               BODYPOS_ARM,           7);
  bodyAddPosition(body, "left arm",                BODYPOS_ARM,           7);
  bodyAddPosition(body, "about body",              BODYPOS_ABOUT,         0);
  bodyAddPosition(body, "torso",                   BODYPOS_TORSO,        50);
  bodyAddPosition(body, "neck",                    BODYPOS_NECK,          1);
  bodyAddPosition(body, "right ear",               BODYPOS_EAR,           0);
  bodyAddPosition(body, "left ear",                BODYPOS_EAR,           0);
  bodyAddPosition(body, "face",                    BODYPOS_FACE,          2);
  bodyAddPosition(body, "head",                    BODYPOS_HEAD,          2);
  bodyAddPosition(body, "floating about head",     BODYPOS_FLOAT,         0);
  //                                                                  ------
  //                                                                    100
  return body;
}


//*****************************************************************************
//
// Functions for body interface. Documentation contained in body.h
//
//*****************************************************************************

char *list_postypes(const BODY_DATA *B, const char *posnames) {
  int i, found, part, num_names = 0;
  char **names  = parse_keywords(posnames, &num_names);
  char types[MAX_BUFFER] = "\0";

  for(i = found = 0; i < num_names; i++) {
    part = bodyGetPart(B, names[i]);
    free(names[i]);
    if(part != BODYPOS_NONE) {
      found++;
      if(found != 1)
	strcat(types, ", ");
      strcat(types, bodyposGetName(part));
    }
  }
  if(names) free(names);

  return strdup(types);
}

const char *bodytypeGetName(int bodytype) {
  return bodytype_list[bodytype];
}

int bodytypeGetNum(const char *bodytype) {
  int i;
  for(i = 0; i < NUM_BODYTYPES; i++)
    if(!strcasecmp(bodytype, bodytype_list[i]))
      return i;
  return BODYTYPE_NONE;
}

const char *bodyposGetName(int bodypos) {
  return bodypos_list[bodypos];
}

int bodyposGetNum(const char *bodypos) {
  int i;
  for(i = 0; i < NUM_BODYPOS; i++)
    if(!strcasecmp(bodypos, bodypos_list[i]))
      return i;
  return BODYPOS_NONE;
}

BODY_DATA *bodyCreate(int bodytype) {
  switch(bodytype) {
  case BODYTYPE_HUMANOID:  return newHumanoidBody();
  case BODYTYPE_DRAGON:    return newDragonBody();
  default:                 return NULL;
  }
}


BODY_DATA *newBody() {
  struct body_data*B = malloc(sizeof(BODY_DATA));
  B->parts = newList();

  return B;
}

void deleteBody(BODY_DATA *B) {
  // delete all of the bodyparts
  deleteListWith(B->parts, deleteBodypart);
  // free us
  free(B);
}

BODY_DATA *bodyCopy(const BODY_DATA *B) {
  BODY_DATA *Bnew = newBody();
  deleteListWith(Bnew->parts, deleteBodypart);
  Bnew->parts = listCopyWith(B->parts, bodypartCopy);

  return Bnew;
}

//
// Find a bodypart on the body with the given name
//
BODYPART *findBodypart(const BODY_DATA *B, const char *pos) {
  LIST_ITERATOR *part_i = newListIterator(B->parts);
  BODYPART *part = NULL;

  ITERATE_LIST(part, part_i)
    if(!strcasecmp(part->name, pos))
      break;
  deleteListIterator(part_i);

  return part;
}

//
// Find a bodypart on the body with of the specified type that
// is not yet equipped with an item
//
BODYPART *findFreeBodypart(BODY_DATA *B, const char *type) {
  int typenum = bodyposGetNum(type);
  if(typenum == BODYPOS_NONE) return NULL;

  LIST_ITERATOR *part_i = newListIterator(B->parts);
  BODYPART *part = NULL;
  ITERATE_LIST(part, part_i)
    if(part->type == typenum && part->equipment == NULL)
      break;
  deleteListIterator(part_i);

  return part;
}

void bodyAddPosition(BODY_DATA *B, const char *pos, int type, int size) {
  BODYPART *part = findBodypart(B, pos);

  // if we've already found the part, just modify it
  if(part) {
    part->type = type;
    part->size = size;
  }
  // otherwise, add a new part
  else
    listPut(B->parts, newBodypart(pos, type, size));
}

bool bodyRemovePosition(BODY_DATA *B, const char *pos) {
  BODYPART *part = findBodypart(B, pos);

  if(!part)
    return FALSE;

  listRemove(B->parts, part);
  deleteBodypart(part);
  return TRUE;
}

int bodyGetPart(const BODY_DATA *B, const char *pos) {
  BODYPART *part = findBodypart(B, pos);
  if(part) return part->type;
  else     return BODYPOS_NONE;
}


double bodyPartRatio(const BODY_DATA *B, const char *pos) {
  LIST_ITERATOR *part_i = newListIterator(B->parts);
  BODYPART        *part = NULL;
  double      part_size = 0.0;
  double      body_size = 0.0;

  // add up all of the weights, and find the weight of our pos
  ITERATE_LIST(part, part_i) {
    body_size += part->size;
    if(is_keyword(pos, part->name, FALSE))
      part_size += part->size;
  }
  deleteListIterator(part_i);

  // to prevent div0, albeit an unlikely event
  return (body_size == 0.0 ? 0 : (part_size / body_size));
}


const char *bodyRandPart(const BODY_DATA *B, const char *pos) {
  LIST_ITERATOR *part_i = newListIterator(B->parts);
  BODYPART *part = NULL;
  char     *name = NULL;
  int   size_sum = 0;
  int   pos_roll = 0;

  // add up all of the weights
  ITERATE_LIST(part, part_i) {
    // if we have a list of positions to draw from, only factor in those
    if(pos && *pos && !is_keyword(pos, part->name, FALSE))
      continue;
    size_sum += part->size;
  }
  // dont' delete the iterator... we use it again below after resetting

  // nothing that can be hit was found
  if(size_sum <= 1) {
    deleteListIterator(part_i);
    return NULL;
  }

  pos_roll = rand_number(1, size_sum);
  
  // find the position the roll corresponds to
  listIteratorReset(part_i);
  ITERATE_LIST(part, part_i) {
    // if we have a list of positions to draw from, only factor in those
    if(pos && *pos && !is_keyword(pos, part->name, FALSE))
      continue;
    pos_roll -= part->size;
    if(pos_roll <= 0) {
      name = part->name;
      break;
    }
  }
  deleteListIterator(part_i);
  return name;
}


const char **bodyGetParts(const BODY_DATA *B, bool sort, int *num_pos) {
  *num_pos = listSize(B->parts);

  const char **parts = malloc(sizeof(char *) * *num_pos);
  LIST_ITERATOR *part_i = newListIterator(B->parts);
  BODYPART *part = NULL;
  int i = 0;

  // if we don't need to sort, this should be fine ...
  if(!sort) {
    ITERATE_LIST(part, part_i)
      parts[i] = part->name;
    deleteListIterator(part_i);
  }
  // take some extra steps to make sure everything is sorted
  else {
    int pos[*num_pos];

    i = 0;
    ITERATE_LIST(part, part_i) {
      parts[i] = part->name;
      pos[i]   = part->type;
      i++;
    }
    deleteListIterator(part_i);

    // now sort everything in the array
    for(i = 0; i < *num_pos; i++) {
      // max_pos in this context is not the position in the array with the
      // highest value. Rather, it is the position in the array whose value
      // corresponds to the highest position on a body, which is negatively
      // related to the value of the position.
      int j, max_pos = i;
      for(j = i+1; j < *num_pos; j++) {
	if(pos[j] < pos[max_pos])
	  max_pos = j;
      }

      // do a shuffle
      if(max_pos != i) {
	const char *tmp_ptr = parts[i];
	parts[i] = parts[max_pos];
	parts[max_pos] = tmp_ptr;

	int tmp_val = pos[i];
	pos[i] = pos[max_pos];
	pos[max_pos] = tmp_val;
      }
    }
  }
  return parts;
}


bool bodyEquipPostypes(BODY_DATA *B, OBJ_DATA *obj, const char *types) {
  int i, num_positions = 0;
  char **pos_list = parse_keywords(types, &num_positions);
  LIST     *parts = newList();
  BODYPART  *part = NULL;

  // make sure we have more than zero positions
  if(num_positions < 1)
    return FALSE;

  // get a list of all open slots in the list provided ...
  // equip them as we go along, incase we more than one of a piece.
  // if we don't do it this way, findFreeBodypart might find the same
  // piece multiple times (e.g. the same ear when it's looking for two ears)
  for(i = 0; i < num_positions; i++) {
    part = findFreeBodypart(B, pos_list[i]);
    if(part && !part->equipment) {
      part->equipment = obj;
      listPut(parts, part);
    }
    free(pos_list[i]);
  }
  if(pos_list) free(pos_list);

  // make sure we supplied valid names
  if(listSize(parts) != num_positions) {
    // remove everything we put on
    while((part = listPop(parts)) != NULL)
      part->equipment = NULL;
    deleteList(parts);
    return FALSE;
  }

  deleteList(parts);
  return TRUE;
}


bool bodyEquipPosnames(BODY_DATA *B, OBJ_DATA *obj, const char *positions) {
  int i, num_positions = 0;
  char **pos_list = parse_keywords(positions, &num_positions);
  LIST  *parts = newList();
  BODYPART *part = NULL;

  // make sure we have more than zero positions
  if(num_positions < 1)
    return FALSE;

  // get a list of all open slots in the list provided
  for(i = 0; i < num_positions; i++) {
    part = findBodypart(B, pos_list[i]);
    if(part && !part->equipment) listPut(parts, part);
    free(pos_list[i]);
  }
  if(pos_list) free(pos_list);

  // make sure we supplied valid names
  if(listSize(parts) != num_positions || listSize(parts) < 1) {
    deleteList(parts);
    return FALSE;
  }

  // fill in all of the parts that need to be filled
  while( (part = listPop(parts)) != NULL)
    part->equipment = obj;
  deleteList(parts);
  return TRUE;
}

const char *bodyEquippedWhere(BODY_DATA *B, OBJ_DATA *obj) {
  static char buf[SMALL_BUFFER];
  *buf = '\0';

  LIST_ITERATOR *part_i = newListIterator(B->parts);
  BODYPART *part = NULL;
  // go through the list of all parts, and print the name of any one
  // with the piece of equipment on it, onto the buf
  ITERATE_LIST(part, part_i) {
    if(part->equipment == obj) {
      // if we've already printed something, add a comma
      if(*buf)
	strcat(buf, ", ");
      strcat(buf, part->name);
    }
  }
  deleteListIterator(part_i);
  return buf;
}

OBJ_DATA *bodyGetEquipment(BODY_DATA *B, const char *pos) {
  BODYPART *part = findBodypart(B, pos);
  return part->equipment;
}

bool bodyUnequip(BODY_DATA *B, const OBJ_DATA *obj) {
  LIST_ITERATOR *part_i = newListIterator(B->parts);
  BODYPART      *part   = NULL;
  bool           found  = FALSE;

  ITERATE_LIST(part, part_i) {
    if(part->equipment == obj) {
      part->equipment = NULL;
      found = TRUE;
    }
  }
  deleteListIterator(part_i);

  return found;
}

LIST *bodyGetAllEq(BODY_DATA *B) {
  LIST *equipment = newList();
  LIST_ITERATOR *part_i = newListIterator(B->parts);
  BODYPART *part = NULL;

  ITERATE_LIST(part, part_i)
    if(part->equipment)
      listPut(equipment, part->equipment);
  deleteListIterator(part_i);
  return equipment;
}

LIST *bodyUnequipAll(BODY_DATA *B) {
  LIST *equipment = newList();
  LIST_ITERATOR *part_i = newListIterator(B->parts);
  BODYPART *part = NULL;

  ITERATE_LIST(part, part_i) {
    if(part->equipment) {
      listPut(equipment, part->equipment);
      part->equipment = NULL;
    }
  }
  deleteListIterator(part_i);
  return equipment;
}

int numBodyparts(const BODY_DATA *B) {
  return listSize(B->parts);
}