#ifndef __ALIAS_H
#define __ALIAS_H
//*****************************************************************************
//
// alias.h
//
// Aliases allow a player to set up one command as another (e.g. "eat bread"
// as "food").
//
//*****************************************************************************

//
// This must be put at the top of mud.h so the rest of the MUD knows that
// we've got the alias module installed
// #define MODULE_ALIAS
//

//
// prepare aliases for use
void init_aliases();

//
// tries to treat the command as an alias. If we succeed, return true. In this
// case, the function calling this one should terminate
bool try_alias(CHAR_DATA *ch, char *command, char *arg, bool scripts_ok);

//
// for use by socket.c to see if we need to do anything special w/ aliases
int charGetAliasesQueued(CHAR_DATA *ch);
void charSetAliasesQueued(CHAR_DATA *ch, int amnt);

#endif // __ALIAS_H
