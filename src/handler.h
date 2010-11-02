#ifndef __HANDLER_H
#define __HANDLER_H
//*****************************************************************************
//
// handler.h
//
// this header contains definitions for all of the "handling" functions;
// functions that move things from place to place. e.g. adding and removing
// characters from rooms, as well as objects and the like.
//
//*****************************************************************************




// these should be called whenever a new mob/char/obj is
// added to to the game, or leaves (deleted or quits)
void      char_to_game      (CHAR_DATA *ch);
void      char_from_game    (CHAR_DATA *ch);
void      obj_to_game       (OBJ_DATA  *obj);
void      obj_from_game     (OBJ_DATA  *obj);


// all of these things require that the character(s) and object(s) have
// the right spatial relations to eachtoher (e.g. do_give requires the
// object be in the char's inventory and the receiver to be in the same room
// as the character, do_get requires the object be on the ground of the room 
// the character is in)
void do_remove (CHAR_DATA *ch, OBJ_DATA *obj);
void do_wear   (CHAR_DATA *ch, OBJ_DATA *obj, const char *where);
void do_drop   (CHAR_DATA *ch, OBJ_DATA *obj);
void do_give   (CHAR_DATA *ch, CHAR_DATA *recv, OBJ_DATA *obj);
void do_get    (CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container);
void do_put    (CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container);

void      obj_from_char       (OBJ_DATA *obj);
void      obj_from_obj        (OBJ_DATA *obj);
void      obj_from_room       (OBJ_DATA *obj);

void      obj_to_char         (OBJ_DATA *obj, CHAR_DATA *ch);
void      obj_to_obj          (OBJ_DATA *obj, OBJ_DATA *to);
void      obj_to_room         (OBJ_DATA *obj, ROOM_DATA *room);

void      char_from_room      (CHAR_DATA *ch);
void      char_to_room        (CHAR_DATA *ch, ROOM_DATA *room);
void      char_from_furniture (CHAR_DATA *ch);
void      char_to_furniture   (CHAR_DATA *ch, OBJ_DATA *furniture);

bool      try_equip         (CHAR_DATA *ch, OBJ_DATA *obj, const char *poslist);
bool      try_unequip       (CHAR_DATA *ch, OBJ_DATA *obj);
void      unequip_all       (CHAR_DATA *ch);


#define FOUND_NONE                   0
#define FOUND_EXIT                   1
#define FOUND_CHAR                   2
#define FOUND_OBJ                    3
#define FOUND_EDESC                  4
#define FOUND_IN_OBJ                 5 // useful for portals and containers
#define FOUND_LIST                   6 // returns a list of things ... may
                                       // occur when looking for all.XXX
                                       // will only be returned if there is only
                                       // FIND_TYPE to look for. The list must 
                                       // be deleted afterwards

#define FIND_TYPE_CHAR         (1 << 0)
#define FIND_TYPE_OBJ          (1 << 1)
#define FIND_TYPE_EXIT         (1 << 2)
#define FIND_TYPE_EDESC        (1 << 3)
#define FIND_TYPE_IN_OBJ       (1 << 4)

#define FIND_TYPE_ALL          (FIND_TYPE_CHAR | FIND_TYPE_OBJ | \
				FIND_TYPE_EXIT | FIND_TYPE_EDESC | \
				FIND_TYPE_IN_OBJ)


#define FIND_SCOPE_ROOM        (1 << 10)
#define FIND_SCOPE_INV         (1 << 11)
#define FIND_SCOPE_WORN        (1 << 12)
#define FIND_SCOPE_WORLD       (1 << 13)

#define FIND_SCOPE_VISIBLE     (1 << 18)


#define FIND_SCOPE_IMMEDIATE   (FIND_SCOPE_ROOM | FIND_SCOPE_INV | \
				FIND_SCOPE_WORN | FIND_SCOPE_VISIBLE)
#define FIND_SCOPE_ALL         (FIND_SCOPE_ROOM | FIND_SCOPE_INV | \
			        FIND_SCOPE_WORN | FIND_SCOPE_WORLD)

//
// Searches the looker's scope of vision for a thing in <find_types> with <arg>
// as the target. Sets found_type to the type of item returned. 
// arg can be manipulated in various ways, such as adding an occurance number
// to the start of arg (e.g. 2.woman) or specifying scopes of search
// (e.g. woman in bed) or combinations of the two (e.g. 2.woman in 6.bed)
// if all_ok is true, then it is possible to return a list of things
// (all of one type) if someone uses something like all.woman
//
void *generic_find(CHAR_DATA *looker, char *arg,
		   bitvector_t find_types, 
		   bitvector_t find_scope, 
		   bool all_ok, int *found_type);

void *find_specific(CHAR_DATA *looker,
		    char *at, char *on, char *in,
		    bitvector_t find_types,
		    bitvector_t find_scope,
		    bool all_ok, int *found_type);

#endif // __HANDLER_H