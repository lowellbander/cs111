// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <stdlib.h>
#include <stdio.h>
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
      c->status = system(*(c->u.word));
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
    default:
    {
      // should never happen
      // could probably take this out
      error(1, 0, "invalid command type\n");
      break; // this line prob not necessary either
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

  error (1, 0, "command execution not yet implemented");
}
