// UCLA CS 111 Lab 1 command execution

#include "alloc.h"
#include "command.h"
#include "command-internals.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}

int execute (command_t c)
{
  switch (c->type)
  {
    case SIMPLE_COMMAND:
    {
      int len = sizeof(*(c->u.word)) + sizeof(*(c->input)) + sizeof(*(c->output)) + 2;
      char* word = checked_malloc(len);
      strcat(word, *(c->u.command[0]->u.word));
      if (c->input != NULL)
      {
        strcat(word, "<");
        strcat(word, c->input);
      }
      if (c->output != NULL)
      {
        strcat(word, ">");
        strcat(word, c->output);
      }
      c->status = system(word);
      break;
    }
    case SUBSHELL_COMMAND:
    {
      c->u.subshell_command->status = execute(c->u.subshell_command);
      break;
    }
    case SEQUENCE_COMMAND:
    {
      c->u.command[0]->status = execute(c->u.command[0]);
      c->u.command[1]->status = execute(c->u.command[1]);
      break;
    }
    case AND_COMMAND:
    {
      c->u.command[0]->status = execute(c->u.command[0]);
      if (c->u.command[0]->status == 0)
        c->u.command[1]->status = execute(c->u.command[1]);
      break;
    }
    case OR_COMMAND:
    {
      c->u.command[0]->status = execute(c->u.command[0]);
      if (c->u.command[0]->status != 0)
        c->u.command[1]->status = execute(c->u.command[1]);
      break;
    }
    case PIPE_COMMAND:
    {
      int len = sizeof(*(c->u.command[0]->u.word)) + sizeof(*(c->u.command[1]->u.word)) + 1;
      char* word = checked_malloc(len);
      printf("len: %i\n", len);
      strcat(word, *(c->u.command[0]->u.word));
      strcat(word, "|");
      strcat(word, *(c->u.command[1]->u.word));
      c->status = system(word);
      break;
    }
  }
  return c->status;
}

void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */

  execute(c);
}
