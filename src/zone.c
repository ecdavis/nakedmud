//*****************************************************************************
//
// zone.c
//
// A zone is like a stand-alone module, or encounter, within the game (e.g.
// a city in the world), and contains all of the NPCs, objects, rooms, scripts,
// etc... needed for the zone to be complete.
//
//*****************************************************************************

#include "mud.h"
#include "storage.h"
#include "object.h"
#include "character.h"
#include "room.h"
#include "utils.h"
#include "auxiliary.h"
#include "dialog.h"
#include "zone.h"


//*****************************************************************************
// mandatory modules
//*****************************************************************************
#include "scripts/script.h"


struct zone_data {
  char   *name;
  char   *editors;
  BUFFER *desc;

  WORLD_DATA     *world;
  PROPERTY_TABLE *rooms;
  PROPERTY_TABLE *scripts;
  PROPERTY_TABLE *dialogs;
  PROPERTY_TABLE *mob_protos;
  PROPERTY_TABLE *obj_protos;

  int vnum;
  int min;
  int max;

  int pulse_timer;  // the timer duration
  int pulse;        // how far down have we gone?

  HASHTABLE            * auxiliary_data; // additional data installed on us
};


ZONE_DATA *newZone(int vnum, int min, int max) {
  ZONE_DATA *zone = malloc(sizeof(ZONE_DATA));
  zone->name    = strdup("");
  zone->desc    = newBuffer(1);
  zone->editors = strdup("");
  zone->vnum    = vnum;
  zone->min     = min;
  zone->max     = max;

  zone->pulse_timer = -1; // never resets
  zone->pulse       = -1;

  zone->world = NULL;
  // maximum of about 5 rooms/bucket
  zone->rooms      = newPropertyTable(roomGetVnum,   1 + (max-min)/5);
  zone->scripts    = newPropertyTable(scriptGetVnum, 1 + (max-min)/5);  
  zone->dialogs    = newPropertyTable(dialogGetVnum, 1 + (max-min)/5);
  zone->mob_protos = newPropertyTable(charGetVnum,    1 + (max-min)/5);
  zone->obj_protos = newPropertyTable(objGetVnum,    1 + (max-min)/5);

  zone->auxiliary_data = newAuxiliaryData(AUXILIARY_TYPE_ZONE);

  return zone;
}

ZONE_DATA *zoneCopy(ZONE_DATA *zone) {
  ZONE_DATA *newzone = newZone(zone->vnum, zone->min, zone->max);
  zoneCopyTo(zone, newzone);
  return newzone;
}

void zoneCopyTo(ZONE_DATA *from, ZONE_DATA *to) {
  zoneSetName(to, zoneGetName(from));
  zoneSetDesc(to, zoneGetDesc(from));
  zoneSetEditors(to, zoneGetEditors(from));
  to->vnum = from->vnum;
  to->min  = from->min;
  to->max  = from->max;
  to->pulse_timer = from->pulse_timer;
  to->pulse = from->pulse;
  auxiliaryDataCopyTo(from->auxiliary_data, to->auxiliary_data);
}

void deleteZone(ZONE_DATA *zone){ 
  if(zone->name)    free(zone->name);
  if(zone->desc)    deleteBuffer(zone->desc);
  if(zone->editors) free(zone->editors);

  deletePropertyTable(zone->rooms);
  deletePropertyTable(zone->scripts);
  deletePropertyTable(zone->dialogs);
  deletePropertyTable(zone->mob_protos);
  deletePropertyTable(zone->obj_protos);

  deleteAuxiliaryData(zone->auxiliary_data);

  //*******************************************************************
  // The only time we're deleting a zone is when we're editing the copy
  // in OLC. So, We don't want to delete any of the zone's content when
  // we delete it.
  //*******************************************************************
    
  free(zone);
}


//
// Pulse a zone. i.e. decrement it's reset timer. When the timer hits 0,
// set it back to the max, and reset everything in the zone
//
void zonePulse(ZONE_DATA *zone){ 
  zone->pulse--;
  if(zone->pulse == 0) {
    zone->pulse = zone->pulse_timer;

    // do a bunch of resetting, and stuff
    int i;
    for(i = zone->min; i <= zone->max; i++) {
      ROOM_DATA *room = propertyTableGet(zone->rooms, i);
      if(room) roomReset(room);
    }
  }
}

void zoneForceReset(ZONE_DATA *zone) {
  zone->pulse = 1;
  zonePulse(zone);
}


//
// The generic function for reading all of the data for one type of thing
// in a zone (e.g. room, object, mobile, etc...). Pulls out the list in
// the storage set, and parses each element of the list, adding it to the zone.
//
void zoneReadData(ZONE_DATA *zone, STORAGE_SET *set, void *putter,void *reader){
  void  *(* put_func)(ZONE_DATA *, void *) = putter;
  LIST *list = gen_read_list(read_list(set, "list"), reader);
  void  *elem = NULL;
  while( (elem = listPop(list)) != NULL)
    put_func(zone, elem);
  deleteList(list);
}


ZONE_DATA *zoneLoad(const char *dirpath) {
  ZONE_DATA *zone = newZone(0, 0, 1);
  char fname[SMALL_BUFFER];

  // first, load all of the zone data
  sprintf(fname, "%s/zone", dirpath);
  STORAGE_SET *set = storage_read(fname);
  zone->vnum        = read_int(set, "vnum");
  zone->min         = read_int(set, "min");
  zone->max         = read_int(set, "max");
  zone->pulse_timer = read_int(set, "pulse_timer");
  zoneSetName(zone,   read_string(set, "name"));
  zoneSetDesc(zone,   read_string(set, "desc"));
  zoneSetEditors(zone,read_string(set, "editors"));

  deleteAuxiliaryData(zone->auxiliary_data);
  zone->auxiliary_data = auxiliaryDataRead(read_set(set, "auxiliary"), 
					   AUXILIARY_TYPE_ZONE);

  storage_close(set);

  // now, load all of the content data
  sprintf(fname, "%s/rooms", dirpath);
  set = storage_read(fname);
  zoneReadData(zone, set, zoneAddRoom, roomRead);
  storage_close(set);

  sprintf(fname, "%s/mobs", dirpath);
  set = storage_read(fname);
  zoneReadData(zone, set, zoneAddMob, charRead);
  storage_close(set);

  sprintf(fname, "%s/objs", dirpath);
  set = storage_read(fname);
  zoneReadData(zone, set, zoneAddObj, objRead);
  storage_close(set);

  sprintf(fname, "%s/dialogs", dirpath);
  set = storage_read(fname);
  zoneReadData(zone, set, zoneAddDialog, dialogRead);
  storage_close(set);

  sprintf(fname, "%s/scripts", dirpath);
  set = storage_read(fname);
  zoneReadData(zone, set, zoneAddScript, scriptRead);
  storage_close(set);
  return zone;
}


//
// The generic function for storing all of the data contained within a 
// zone (e.g. objects, mobiles, rooms, etc...). Goes through each one
// and creates a storage set containing a list with all the things.
// returns the storage set.
//
STORAGE_SET *zoneStoreData(ZONE_DATA *zone, void *getter, void *storer) {
  void *(* get_func)(ZONE_DATA *, int) = getter;
  STORAGE_SET  *(* store_func)(void *) = storer;

  STORAGE_SET       *set = new_storage_set();
  STORAGE_SET_LIST *list = new_storage_list();
  void             *data = NULL;
  store_list(set, "list", list);

  int i = zoneGetMinBound(zone);
  for(; i <= zoneGetMaxBound(zone); i++) {
    if( (data = get_func(zone, i)) == NULL)
	continue;
    storage_list_put(list, store_func(data));
  }
  return set;
}


//
// the new zone saving function
//
bool zoneSave(ZONE_DATA *zone, const char *dirpath) {
  char fname[MAX_BUFFER];
  
  // first, for our zone data
  sprintf(fname, "%s/zone", dirpath);
  STORAGE_SET *set = new_storage_set();
  store_int   (set, "vnum",        zone->vnum);
  store_int   (set, "min",         zone->min);
  store_int   (set, "max",         zone->max);
  store_int   (set, "pulse_timer", zone->pulse_timer);
  store_string(set, "name",        zone->name);
  store_string(set, "desc",        bufferString(zone->desc));
  store_string(set, "editors",     zone->editors);
  store_set   (set, "auxiliary",   auxiliaryDataStore(zone->auxiliary_data));
  storage_write(set, fname);
  storage_close(set);

  set = zoneStoreData(zone, zoneGetRoom, roomStore);
  sprintf(fname, "%s/rooms", dirpath);
  storage_write(set, fname);

  set = zoneStoreData(zone, zoneGetObj, objStore);
  sprintf(fname, "%s/objs", dirpath);
  storage_write(set, fname);

  set = zoneStoreData(zone, zoneGetMob, charStore);
  sprintf(fname, "%s/mobs", dirpath);
  storage_write(set, fname);

  set = zoneStoreData(zone, zoneGetDialog, dialogStore);
  sprintf(fname, "%s/dialogs", dirpath);
  storage_write(set, fname);

  set = zoneStoreData(zone, zoneGetScript, scriptStore);
  sprintf(fname, "%s/scripts", dirpath);
  storage_write(set, fname);

  return TRUE;
}


//*****************************************************************************
//
// add and remove functions
//
//*****************************************************************************

//
// The generic function for adding data into a zone.
//
bool zoneAdd(ZONE_DATA *zone, PROPERTY_TABLE *table, const char *datatype,
	     void *data, int vnum) {
  if(vnum < zone->min || vnum > zone->max)
    log_string("ERROR: tried to add %s %d to zone %d - "
	       "vnum out of bounds for zone (%d, %d)!",
	       datatype, vnum, zone->vnum, zone->min, zone->max);
  else if(propertyTableGet(table, vnum) != NULL)
    log_string("ERROR: tried to add %s %d to zone %d - "
	       "%s with vnum already exists in zone!",
	       datatype, vnum, zone->vnum, datatype);
  else {
    propertyTablePut(table, data);
    return TRUE;
  }
  return FALSE;
}

void zoneAddRoom(ZONE_DATA *zone, ROOM_DATA *room) { 
  zoneAdd(zone, zone->rooms, "room", room, roomGetVnum(room));
}

void zoneAddMob(ZONE_DATA *zone, CHAR_DATA *mob) {
  zoneAdd(zone, zone->mob_protos, "mobile", mob, charGetVnum(mob)); 
}

void zoneAddObj(ZONE_DATA *zone, OBJ_DATA *obj) {
  zoneAdd(zone, zone->obj_protos, "object", obj, objGetVnum(obj));
}

void zoneAddScript(ZONE_DATA *zone, SCRIPT_DATA *script) {
  zoneAdd(zone, zone->scripts, "script", script, scriptGetVnum(script));
}

void zoneAddDialog(ZONE_DATA *zone, DIALOG_DATA *dialog) {
  zoneAdd(zone, zone->dialogs, "dialog", dialog, dialogGetVnum(dialog));
}

//
// Generic zone remove function
// 
void *zoneRemove(ZONE_DATA *zone, PROPERTY_TABLE *table, 
		 const char *datatype, int vnum) {
  if(vnum < zone->min || vnum > zone->max) {
    log_string("ERROR: tried to remove %s %d from zone %d - "
	       "vnum out of bounds for zone!",
	       datatype, vnum, zone->vnum);
    return NULL;
  }
  else
    return propertyTableRemove(table, vnum);
}

ROOM_DATA *zoneRemoveRoom(ZONE_DATA *zone, int room){ 
  return zoneRemove(zone, zone->rooms, "room", room);
}

CHAR_DATA *zoneRemoveMob(ZONE_DATA *zone, int mob){ 
  return zoneRemove(zone, zone->mob_protos, "mob", mob);
}

OBJ_DATA *zoneRemoveObj(ZONE_DATA *zone, int obj){ 
  return zoneRemove(zone, zone->obj_protos, "obj", obj);
}

SCRIPT_DATA *zoneRemoveScript(ZONE_DATA *zone, int script) { 
  return zoneRemove(zone, zone->scripts, "script", script);
}

DIALOG_DATA *zoneRemoveDialog(ZONE_DATA *zone, int dialog) { 
  return zoneRemove(zone, zone->dialogs, "dialog", dialog);
}



//*****************************************************************************
//
// get and set functions for zones
//
//*****************************************************************************
bool canEditZone(ZONE_DATA *zone, CHAR_DATA *ch) {
  return (!charIsNPC(ch) && is_keyword(zone->editors, charGetName(ch), FALSE));
}

int zoneGetVnum(ZONE_DATA *zone) { 
  return zone->vnum;
}

int zoneGetMinBound(ZONE_DATA *zone) { 
  return zone->min;
}

int zoneGetMaxBound(ZONE_DATA *zone) { 
  return zone->max;
}

void *zoneGetAuxiliaryData(const ZONE_DATA *zone, char *name) {
  return hashGet(zone->auxiliary_data, name);
}

int getFreeRoomVnum(ZONE_DATA *zone) {
  int i;
  for(i = zone->min; i <= zone->max; i++)
    if(!zoneGetRoom(zone, i))
      return i;
  return NOWHERE;
}

int zoneGetPulseTimer(ZONE_DATA *zone) { 
  return zone->pulse_timer;
}

int zoneGetPulse(ZONE_DATA *zone) { 
  return zone->pulse;
}

WORLD_DATA *zoneGetWorld(ZONE_DATA *zone) { 
  return zone->world;
}

const char *zoneGetName(ZONE_DATA *zone) { 
  return zone->name;
}

const char *zoneGetDesc(ZONE_DATA *zone) { 
  return bufferString(zone->desc);
}

const char *zoneGetEditors(ZONE_DATA *zone) {
  return zone->editors;
}

BUFFER *zoneGetDescBuffer(ZONE_DATA *zone) {
  return zone->desc;
}

//
// Generic get function for zones
//
void *zoneGet(ZONE_DATA *zone, PROPERTY_TABLE *table, const char *datatype,
	      int vnum) {
  if(vnum < zone->min || vnum > zone->max) {
    log_string("ERROR: tried to get %s %d from zone %d - "
	       "vnum out of bounds for zone!",
	       datatype, vnum, zone->vnum);
    return NULL;
  }
  else
    return propertyTableGet(table, vnum);
}

ROOM_DATA *zoneGetRoom(ZONE_DATA *zone, int room) {
  return zoneGet(zone, zone->rooms, "room", room);
}

CHAR_DATA *zoneGetMob(ZONE_DATA *zone, int mob) {
  return zoneGet(zone, zone->mob_protos, "mob", mob);
}

OBJ_DATA *zoneGetObj(ZONE_DATA *zone, int obj) {
  return zoneGet(zone, zone->obj_protos, "obj", obj);
}

SCRIPT_DATA *zoneGetScript(ZONE_DATA *zone, int script) {
  return zoneGet(zone, zone->scripts, "script", script);
};

DIALOG_DATA *zoneGetDialog(ZONE_DATA *zone, int dialog) {
  return zoneGet(zone, zone->dialogs, "dialog", dialog);
};

void zoneSetVnum(ZONE_DATA *zone, int vnum) { 
  zone->vnum = vnum;
}

void zoneSetMinBound(ZONE_DATA *zone, int min) { 
  zone->min = min;
}

void zoneSetMaxBound(ZONE_DATA *zone, int max) { 
  zone->max = max;
}

void zoneSetPulseTimer(ZONE_DATA *zone, int timer) { 
  // if we normally do not reset, change that
  if(zone->pulse_timer < 0)
    zone->pulse = timer;
  zone->pulse_timer = timer;
}

void zoneSetPulse(ZONE_DATA *zone, int pulse_left) { 
  zone->pulse = pulse_left;
}

void zoneSetWorld(ZONE_DATA *zone, WORLD_DATA *world) { 
  zone->world = world;
}

void zoneSetName(ZONE_DATA *zone, const char *name) { 
  if(zone->name) free(zone->name);
  zone->name = (name ? strdup(name) : strdup("\0"));
}

void zoneSetDesc(ZONE_DATA *zone, const char *desc) { 
  bufferClear(zone->desc);
  bufferCat(zone->desc, desc);
}

void zoneSetEditors(ZONE_DATA *zone, const char *names) {
  if(zone->editors) free(zone->editors);
  zone->editors = strdup( (names ? names : ""));
}
