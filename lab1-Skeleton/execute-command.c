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

//char* build_command_string()

void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */

  // traverse the command tree and execute commands with system()
  //system("echo hello, world!");

  switch(c->type)
  {
    case SIMPLE_COMMAND:
    {
      char* word = *(c->u.word);
      printf("command: %s\n", word);
      system(word);
      break;
    }
    case AND_COMMAND:
    case SEQUENCE_COMMAND:
    case OR_COMMAND:
    case PIPE_COMMAND:
    {
      printf("cases not handled\n");
      break;
    }
    default:
    {
      printf("fell through swtich\n");
      break;
    }
  }

  error (1, 0, "command execution not yet implemented");
}
