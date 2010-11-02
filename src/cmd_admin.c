//*****************************************************************************
//
// cmd_admin.c
//
// commands and procedures available only to admins.
//
//*****************************************************************************

#include "mud.h"
#include "world.h"
#include "inform.h"
#include "character.h"
#include "handler.h"
#include "utils.h"
#include "log.h"
#include "socket.h"
#include "save.h"
#include "event.h"
#include "action.h"
#include "storage.h"


//
// BOOM! Shut down the MUD
//
COMMAND(cmd_shutdown) {
  shut_down = TRUE;
}


//
// Go to a specific room, object, or character in the game. Rooms are referenced
// by vnum. Everything else is referenced by name.
//   usage: goto [thing]
//
//   examples:
//     goto 100             go to room number 100
//     goto jim             go to an object/person named jim
//
COMMAND(cmd_goto) {
  if(!arg || !*arg)
    send_to_char(ch, "Where would you like to go to?\r\n");
  // we're trying to go to a specific room number
  else if(isdigit(*arg)) {
    ROOM_DATA *room = worldGetRoom(gameworld, atoi(arg));

    if(!room)
      send_to_char(ch, "No such room exists.\r\n");
    else if(room == charGetRoom(ch))
      send_to_char(ch, "You're already here, boss.\r\n");
    else {
      message(ch, NULL, NULL, NULL, TRUE, TO_ROOM | TO_NOTCHAR,
	      "$n disappears in a puff of smoke.");
      char_from_room(ch);
      char_to_room(ch, room);
      look_at_room(ch, room);
      message(ch, NULL, NULL, NULL, TRUE, TO_ROOM | TO_NOTCHAR,
	      "$n arrives in a puff of smoke.");
    }
  }

  // find the character we're trying to go to
  else {
    void *tgt = generic_find(ch, arg, 
			     FIND_TYPE_CHAR,
			     FIND_SCOPE_WORLD | FIND_SCOPE_VISIBLE,
			     FALSE, NULL);

    if(ch == tgt)
      send_to_char(ch, "You're already here, boss.\r\n");
    else if(tgt != NULL) {
      message(ch, NULL, NULL, NULL, TRUE, TO_ROOM | TO_NOTCHAR,
	      "$n disappears in a puff of smoke.");
      char_from_room(ch);
      char_to_room(ch, charGetRoom(tgt));
      look_at_room(ch, charGetRoom(ch));
      message(ch, NULL, NULL, NULL, TRUE, TO_ROOM | TO_NOTCHAR,
	      "$n arrives in a puff of smoke.");
    }
    else
      send_to_char(ch, "Who were you trying to go to?\r\n");
  }
}


//
// The opposite of goto. Instead of moving to a specified location, it
// takes the target to the user.
//   usage: transfer [player]
//
COMMAND(cmd_transfer) {
  if(!arg || !*arg)
    send_to_char(ch, "Who would you like to transfer?\r\n");
  else {
    void *tgt = generic_find(ch, arg, 
			     FIND_TYPE_CHAR,
			     FIND_SCOPE_WORLD | FIND_SCOPE_VISIBLE,
			     FALSE, NULL);

    if(ch == tgt)
      send_to_char(ch, "You're already here, boss.\r\n");
    else if(charGetRoom(ch) == charGetRoom(tgt))
      send_to_char(ch, "They're already here.\r\n");
    else if(tgt != NULL) {
      message(ch, tgt, NULL, NULL, TRUE, TO_VICT,
	      "$n has transferred you!");
      message(tgt, NULL, NULL, NULL, TRUE, TO_ROOM | TO_NOTCHAR,
	      "$n disappears in a puff of smoke.");
      char_from_room(tgt);
      char_to_room(tgt, charGetRoom(ch));
      look_at_room(tgt, charGetRoom(tgt));
      message(tgt, NULL, NULL, NULL, TRUE, TO_ROOM | TO_NOTCHAR,
	      "$n arrives in a puff of smoke.");
    }
    else
      send_to_char(ch, "Who were you trying to transfer?\r\n");
  }
}


//
// Perform a copyover
//
COMMAND(cmd_copyover) { 
  FILE *fp;
  SOCKET_DATA *dsock;
  char buf[100];
  char control_buf[20];
  char port_buf[20];
  LIST_ITERATOR *sock_i = newListIterator(socket_list);
  
  if ((fp = fopen(COPYOVER_FILE, "w")) == NULL)
  {
    text_to_char(ch, "Copyover file not writeable, aborted.\n\r");
    return;
  }

  sprintf(buf, "\n\r <*>            The world starts spinning             <*>\n\r");

  /* For each playing descriptor, save its state */
  //  for (dsock = dsock_list; dsock ; dsock = dsock_next)
  ITERATE_LIST(dsock, sock_i) {
    compressEnd(dsock, dsock->compressing, FALSE);
    if (dsock->state != STATE_PLAYING) {
      text_to_socket(dsock, "\r\nSorry, we are rebooting. Come back in a few minutes.\r\n");
      close_socket(dsock, FALSE);
    }
    else {
      fprintf(fp, "%d %s %s\n",
	      dsock->control, charGetName(dsock->player), dsock->hostname);
      /* save the player */
      save_player(dsock->player);
      text_to_socket(dsock, buf);
    }
  }
  deleteListIterator(sock_i);
  
  fprintf (fp, "-1\n");
  fclose (fp);


  /* close any pending sockets */
  recycle_sockets();
  
  /* exec - descriptors are inherited */
  sprintf(control_buf, "%d", control);
  sprintf(port_buf, "%d", mudport);
  execl(EXE_FILE, "NakedMud", "-copyover", control_buf, port_buf, NULL);

  /* Failed - sucessful exec will not return */
  text_to_char(ch, "Copyover FAILED!\n\r");
}


//
// show a list of all the PCs who are linkdead
//
COMMAND(cmd_linkdead) {
  CHAR_DATA *xMob;
  char buf[MAX_BUFFER];
  bool found = FALSE;
  LIST_ITERATOR *mob_i = newListIterator(mobile_list);

  ITERATE_LIST(xMob, mob_i) {
    if (!(charIsNPC(xMob) || charGetSocket(xMob))) {
      sprintf(buf, "%s is linkdead.\n\r", charGetName(xMob));
      text_to_char(ch, buf);
      found = TRUE;
    }
  }
  deleteListIterator(mob_i);

  if (!found)
    text_to_char(ch, "Noone is currently linkdead.\n\r");
}


//
// List all of the non-player commands the character has access to
//
COMMAND(cmd_wizhelp) {
  show_commands(ch, LEVEL_BUILDER, charGetLevel(ch));
}


//
// Turn on/off immortal invisibility
//
COMMAND(cmd_invis) {
  int level = charGetLevel(ch); // default to our level
  if(arg && *arg && isdigit(*arg))
    level = atoi(arg);

  // make sure we're not trying to go invisible at a level higher than us
  if(level > charGetLevel(ch)) {
    send_to_char(ch, "You cannot go invisibile to a level higher than yours.\r\n");
    return;
  }

  // see if we're trying to go visible
  if(level == 0 && charGetImmInvis(ch) > 0) {
    send_to_char(ch, "Invisibility turned off.\r\n");
    charSetImmInvis(ch, 0);
    message(ch, NULL, NULL, NULL, TRUE, TO_ROOM | TO_NOTCHAR,
	    "$n slowly fades into existence.");    
  }
  // or invisible
  else if(level > 0) {
    message(ch, NULL, NULL, NULL, TRUE, TO_ROOM | TO_NOTCHAR,
	    "$n slowly fades out of existence.");
    charSetImmInvis(ch, level);
    send_to_char(ch, "You are now invisible to anyone below level %d.\r\n",
		 level);
  }
  // we tried to go visible, but we already are visible
  else
    send_to_char(ch, "But you're already visible!\r\n");
}


//
// turn off immortal invisibility
//
COMMAND(cmd_visible) {
  if(charGetImmInvis(ch) == 0)
    send_to_char(ch, "But you're already visible!\r\n");
  else {
    charSetImmInvis(ch, 0);
    message(ch, NULL, NULL, NULL, TRUE, TO_ROOM | TO_NOTCHAR,
	    "$n slowly fades into existence.");
  }
}