/*
 * This file handles command interpreting
 */
#include <sys/types.h>
#include <stdio.h>

/* include main header file */
#include "mud.h"
#include "utils.h"
#include "character.h"
#include "socket.h"
#include "room.h"
#include "movement.h"    // for try_move_special
#include "commands.h"
#include "action.h"

// optional modules
#ifdef MODULE_FACULTY
#include "modules/faculty/faculty.h"
#endif
#ifdef MODULE_SCRIPTS
#include "modules/scripts/script.h"
#endif


// our command table list
LIST   **cmd_table = NULL;

struct cmd_data {
  char      * cmd_name;
  char      * sort_by;
  void     (* cmd_funct)(CHAR_DATA *ch, char *arg, int subcmd); // COMMAND()
  int        subcmd;
  int        min_pos;
  int        max_pos;
  int        level;
  bool       interrupts; // does it interrupt actions?
};


void init_commands() {
  cmd_table = malloc(sizeof(LIST *) * 26); // 1 for each letter
  int i;
  for(i = 0; i < 26; i++)
    cmd_table[i] = newList();

  //***********************************************************************
  // This is for core functions ONLY! If you have a module that adds new
  // functions to the MUD, they should be added in the init_xxx() function
  // associated with your module.
  //***********************************************************************
  add_cmd("north", "n", cmd_move,     DIR_NORTH,    POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("east",  "e", cmd_move,     DIR_EAST,     POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("south", "s", cmd_move,     DIR_SOUTH,    POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("west",  "w", cmd_move,     DIR_WEST,     POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("up",    "u", cmd_move,     DIR_UP,       POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("down",  "d", cmd_move,     DIR_DOWN,     POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("northeast", "na", cmd_move,DIR_NORTHEAST,POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("southeast", "sa",  cmd_move,DIR_SOUTHEAST,POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("southwest", "sb", cmd_move,DIR_SOUTHWEST,POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("northwest", "nb", cmd_move,DIR_NORTHWEST,POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("ne",        "ne", cmd_move,DIR_NORTHEAST,POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("se",        "se", cmd_move,DIR_SOUTHEAST,POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("sw",        "sw", cmd_move,DIR_SOUTHWEST,POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("nw",        "nw", cmd_move,DIR_NORTHWEST,POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );

  // A
  add_cmd("alias",      NULL, cmd_alias,    0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("approach",   NULL, cmd_greet,    0, POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("ask",        NULL, cmd_ask,      0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, FALSE);

  // B
  add_cmd("back",       NULL, cmd_back,     0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_PLAYER, FALSE);
  add_cmd("buildwalk",  NULL, cmd_tog_prf,  SUBCMD_BUILDWALK, 
	  POS_UNCONCIOUS, POS_FLYING, LEVEL_BUILDER, FALSE);

  // C
  add_cmd("chat",       NULL, cmd_chat,     0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_PLAYER, FALSE);
  add_cmd("clear",      NULL, cmd_clear,    0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_PLAYER, FALSE);
  add_cmd("close",      NULL, cmd_close,    0, POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("commands",   NULL, cmd_commands, 0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_PLAYER, FALSE);
  add_cmd("compress",   NULL, cmd_compress, 0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_PLAYER, FALSE);
  add_cmd("copyover",   NULL, cmd_copyover, 0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_ADMIN,   TRUE);

  // D
  add_cmd("delay",      NULL, cmd_delay,    0, POS_SLEEPING, POS_FLYING,
	  LEVEL_PLAYER,  FALSE);
  
  add_cmd("dig",        NULL, cmd_dig,      0, POS_STANDING, POS_FLYING,
	  LEVEL_BUILDER, FALSE);
  add_cmd("dlist",      NULL, cmd_dlist,    0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_BUILDER, FALSE );
  add_cmd("drop",       NULL, cmd_drop,     0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, TRUE );

  // E
  add_cmd("enter",      NULL, cmd_enter,    0, POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("emote",      NULL, cmd_emote,    0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, FALSE);
  add_cmd("equipment",  NULL, cmd_equipment,0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, FALSE);

  // F
  add_cmd("fill",       NULL, cmd_fill,     0, POS_STANDING, POS_FLYING,
	  LEVEL_BUILDER, TRUE );

  // G
  add_cmd("gemote",     NULL, cmd_gemote,   0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_PLAYER, FALSE);
  add_cmd("give",       NULL, cmd_give,     0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("gossip",     NULL, cmd_chat,     0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_PLAYER, FALSE);
  add_cmd("greet",      NULL, cmd_greet,    0, POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("get",        NULL, cmd_get,      0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("goto",       NULL, cmd_goto,     0, POS_STANDING, POS_FLYING,
	  LEVEL_BUILDER, TRUE );

  // H
  add_cmd("help",       NULL, cmd_help,     0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_PLAYER, FALSE);

  // I
  add_cmd("inventory",  NULL, cmd_inventory,0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, FALSE);

  // L
  add_cmd("look",       "l",  cmd_look,     0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, FALSE);
  add_cmd("lock",       NULL,  cmd_lock,    0, POS_STANDING,  POS_FLYING,
	  LEVEL_PLAYER, TRUE);
  add_cmd("land",       NULL, cmd_stand,    0, POS_FLYING,   POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("load",       NULL, cmd_load,     0, POS_SITTING,  POS_FLYING,
	  LEVEL_BUILDER, FALSE);
  add_cmd("linkdead",   NULL, cmd_linkdead, 0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_ADMIN, FALSE);

  // M
  add_cmd("mlist",      NULL, cmd_mlist,    0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_BUILDER, FALSE);
  add_cmd("more",       NULL, cmd_more,     0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_PLAYER, FALSE);

  // O
  add_cmd("olist",      NULL, cmd_olist,    0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_BUILDER, FALSE);
  add_cmd("open",       NULL, cmd_open,     0, POS_STANDING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );

  // P
  add_cmd("put",        NULL, cmd_put,      0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER,  TRUE );
  add_cmd("purge",      NULL, cmd_purge,    0, POS_SITTING,  POS_FLYING,
	  LEVEL_BUILDER, FALSE);
  
  // Q
  add_cmd("quit",       NULL, cmd_quit,     0, POS_SLEEPING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );

  // R
  add_cmd("remove",     NULL, cmd_remove,   0, POS_SITTING, POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("rlist",      NULL, cmd_rlist,    0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_BUILDER, FALSE);

  // S
  add_cmd("say",        NULL, cmd_say,      0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, FALSE);
  add_cmd("save",       NULL, cmd_save,     0, POS_SLEEPING, POS_FLYING,
	  LEVEL_PLAYER, FALSE);
#ifdef MODULE_SCRIPTS
  add_cmd("sclist",     NULL, cmd_sclist,   0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_BUILDER, FALSE);
#endif
  add_cmd("shutdown",   NULL, cmd_shutdown, 0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_ADMIN, TRUE );
  add_cmd("sit",        NULL, cmd_sit,      0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("sleep",      NULL, cmd_sleep,    0, POS_SITTING,  POS_STANDING,
	  LEVEL_PLAYER, FALSE);
  add_cmd("stand",      NULL, cmd_stand,    0, POS_SITTING,  POS_STANDING,
	  LEVEL_PLAYER, TRUE );

  // T
  add_cmd("take",       NULL, cmd_get,      0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("tell",       NULL, cmd_tell,     0, POS_SLEEPING, POS_FLYING,
	  LEVEL_PLAYER, FALSE);

  // U
  add_cmd("unlock",       NULL,  cmd_unlock,    0, POS_STANDING,  POS_FLYING,
	  LEVEL_PLAYER, TRUE);

  // W
  add_cmd("wake",       NULL, cmd_wake,     0, POS_SLEEPING,  POS_SLEEPING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("wear",       NULL, cmd_wear,     0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, TRUE );
  add_cmd("who",        NULL, cmd_who,      0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_PLAYER, FALSE);
  add_cmd("worn",       NULL, cmd_equipment,0, POS_SITTING,  POS_FLYING,
	  LEVEL_PLAYER, FALSE);

  // Z
  add_cmd("zlist",      NULL, cmd_zlist,    0, POS_SITTING,  POS_FLYING,
	  LEVEL_BUILDER, TRUE);
  add_cmd("zreset",     NULL, cmd_zreset,   0, POS_UNCONCIOUS, POS_FLYING,
	  LEVEL_BUILDER, FALSE);
}


int cmdbucket(const char *cmd) {
  if(!isalpha(*cmd))
    return 0;
  return (tolower(*cmd) - 'a');
}

// compare two command by their sort_by variable
int cmdsortbycmp(const struct cmd_data *cmd1, const struct cmd_data *cmd2) {
  return strcasecmp(cmd1->sort_by, cmd2->sort_by);
}

// compare two commands
int cmdcmp(const struct cmd_data *cmd1, const struct cmd_data *cmd2) {
  return strcasecmp(cmd1->cmd_name, cmd2->cmd_name);
}

// check if the cmd matches the command's cmd_name up to the length of cmd
int is_cmd_abbrev(const char *cmd, const struct cmd_data *entry) {
  return strncasecmp(cmd, entry->cmd_name, strlen(cmd));
}

// check if the string matches the command's cmd_name
int is_cmd(const char *cmd, const struct cmd_data *entry) {
  return strcasecmp(cmd, entry->cmd_name);
}

// find a command
struct cmd_data *find_cmd(const char *cmd, bool abbrev_ok) {
  if(abbrev_ok)
    return listGetWith(cmd_table[cmdbucket(cmd)], cmd, is_cmd_abbrev);
  else
    return listGetWith(cmd_table[cmdbucket(cmd)], cmd, is_cmd);
}

void remove_cmd(const char *cmd) {
  listRemoveWith(cmd_table[cmdbucket(cmd)], cmd, is_cmd);
}

void add_cmd(const char *cmd, const char *sort_by,
	     void *func, int subcmd, int min_pos, int max_pos,
	     int min_level, bool interrupts) {
  // if we've already got a command named this, remove it
  if(find_cmd(cmd, FALSE) != NULL)
    remove_cmd(cmd);

  // make our new command data
  struct cmd_data *new_cmd = malloc(sizeof(struct cmd_data));
  new_cmd->cmd_name   = strdup(cmd);
  new_cmd->sort_by    = strdup(sort_by ? sort_by : cmd);
  new_cmd->cmd_funct  = func;
  new_cmd->subcmd     = subcmd;
  new_cmd->min_pos    = min_pos;
  new_cmd->max_pos    = max_pos;
  new_cmd->level      = min_level;
  new_cmd->interrupts = interrupts;

  // and add it in
  listPutWith(cmd_table[cmdbucket(cmd)], new_cmd, cmdsortbycmp);
}


// show the character all of the commands he or she can perform
void show_commands(CHAR_DATA *ch) {
  BUFFER *buf = buffer_new(MAX_BUFFER);
  int i, col = 0;

  // go over all of our buckets
  for(i = 0; i < 26; i++) {
    LIST_ITERATOR *buck_i = newListIterator(cmd_table[i]);
    struct cmd_data *cmd = NULL;

    ITERATE_LIST(cmd, buck_i) {
      if(charGetLevel(ch) < cmd->level)
	continue;
      bprintf(buf, " %-16.16s", cmd->cmd_name);
      if (!(++col % 4))
	bprintf(buf, "\r\n");      
    }
    deleteListIterator(buck_i);
  }

  if (col % 4) bprintf(buf, "\r\n");
  text_to_char(ch, buf->data);
  buffer_free(buf);
}



//
// make sure the character is in a position where
// this can be performed
// 
bool min_pos_ok(CHAR_DATA *ch, int minpos) {
  if(poscmp(charGetPos(ch), minpos) >= 0)
    return TRUE;
  else {
    switch(charGetPos(ch)) {
    case POS_UNCONCIOUS:
      send_to_char(ch, "You cannot do that while unconcious!\r\n");
      break;
    case POS_SLEEPING:
      send_to_char(ch, "Not while sleeping, you won't!\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "You cannot do that while sitting!\r\n");
      break;
    case POS_STANDING:
      // flying is the highest position... we can deduce this message
      send_to_char(ch, "You must be flying to try that.\r\n");
      break;
    case POS_FLYING:
      send_to_char(ch, "That is not possible in any position you can think of.\r\n");
      break;
    default:
      send_to_char(ch, "Your position is all wrong!\r\n");
      log_string("Character, %s, has invalid position, %d.",
		 charGetName(ch), charGetPos(ch));
      break;
    }
    return FALSE;
  }
}


//
// make sure the character is in a position where
// this can be performed
// 
bool max_pos_ok(CHAR_DATA *ch, int minpos) {
  if(poscmp(charGetPos(ch), minpos) <= 0)
    return TRUE;
  else {
    switch(charGetPos(ch)) {
    case POS_UNCONCIOUS:
      send_to_char(ch, "You're still too alive to try that!\r\n");
      break;
    case POS_SLEEPING:
      send_to_char(ch, "Not while sleeping, you won't!\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "You cannot do that while sitting!\r\n");
      break;
    case POS_STANDING:
      send_to_char(ch, "You cannot do that while standing.\r\n");
      break;
    case POS_FLYING:
      send_to_char(ch, "You must land first.\r\n");
      break;
    default:
      send_to_char(ch, "Your position is all wrong!\r\n");
      log_string("Character, %s, has invalid position, %d.",
		 charGetName(ch), charGetPos(ch));
      break;
    }
    return FALSE;
  }
}


//
// expand an alias out, replacing all wildcards with arguments passed in
//
char *expand_alias(const char *alias, const char *arg) {
  char *cmd = strdup(alias);
  replace_string(&cmd, "$*", arg, TRUE);
  return cmd;
}


void handle_cmd_input(SOCKET_DATA *dsock, char *arg) {
  CHAR_DATA *ch;
  if ((ch = dsock->player) == NULL)
    return;
  do_cmd(ch, arg, TRUE, TRUE);
}


void do_cmd(CHAR_DATA *ch, char *arg, bool scripts_ok, bool aliases_ok)  {
  char command[MAX_BUFFER];
  bool found_cmd = FALSE;

  // make sure we've got a command to try
  if(!arg || !*arg)
    return;

  arg = one_arg(arg, command);

  // see if it's an alias
  if(aliases_ok && charGetAlias(ch, command)) {
    char *alias_cmd = expand_alias(charGetAlias(ch, command), arg);
    do_cmd(ch, alias_cmd, scripts_ok, FALSE);
    free(alias_cmd);
    return;
  }


  // check to see if it's a faculty command
#ifdef MODULE_FACULTY
  if(try_use_faculty(ch, command))
    return;
#endif

  // if we've got a command script, and we're not supposed
  // to follow through with our normal command, return out
#ifdef MODULE_SCRIPTS
  if(scripts_ok && try_command_script(ch, command, arg))
    return;
#endif


  // iterate over the commands that would be in our 
  // bucket and find the one that we are trying to use
  LIST_ITERATOR *cmd_i = newListIterator(cmd_table[cmdbucket(command)]);
  struct cmd_data *cmd = NULL;
  ITERATE_LIST(cmd, cmd_i) {
    if (cmd->level > charGetLevel(ch))
      continue;

    if (is_prefix(command, cmd->cmd_name)) {
      found_cmd = TRUE;
      if(min_pos_ok(ch, cmd->min_pos) && max_pos_ok(ch,cmd->max_pos)) {
	if(cmd->interrupts) {
#ifdef MODULE_FACULTY
	  interrupt_action(ch, FACULTY_ALL);
#else
	  interrupt_action(ch, 1);
#endif
	}
	(cmd->cmd_funct)(ch, arg, cmd->subcmd);
      }
      break;
    }
  }
  deleteListIterator(cmd_i);



  if (!found_cmd) {
    // check to see if the character is trying to use
    // a special exit command for the room
    if(roomGetExitSpecial(charGetRoom(ch), command)) {
      if(min_pos_ok(ch, POS_STANDING)) {
#ifdef MODULE_FACULTY
	interrupt_action(ch, FACULTY_ALL);
#else
	interrupt_action(ch, 1);
#endif
	try_move(ch, DIR_NONE, command);
      }
    }
    else
      text_to_char(ch, "No such command.\n\r");
  }
}