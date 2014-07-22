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

int find_command_size (command_t c)
{
  int len = 0;
  switch (c->type)
  {
    case SIMPLE_COMMAND:
    {
      len = sizeof(*(c->u.word)) + sizeof(*(c->input)) 
            + sizeof(*(c->output)) + 2;
      break;
    }
    case SUBSHELL_COMMAND:
    {
      len = 2 + find_command_size(c->u.command[0]);
      break;
    }
    case PIPE_COMMAND:
    case SEQUENCE_COMMAND:
    {
      len = find_command_size(c->u.command[0]) 
            + find_command_size(c->u.command[1]) + 1;
      break;
    }
    case AND_COMMAND:
    case OR_COMMAND:
    {
      len = find_command_size(c->u.command[0]) 
            + find_command_size(c->u.command[1]) + 2;
      break;
    }
  }
  return len;
}

char* build_sys_string (command_t c)
{
  int len = find_command_size(c);
  char* word = checked_malloc(len);
  
  switch (c->type)
  {
    case SIMPLE_COMMAND:
    {
      strcat(word, *(c->u.word));
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
      break;
    }
    case SUBSHELL_COMMAND:
    {
      strcat(word, "(");
      strcat(word, build_sys_string(c->u.subshell_command));
      strcat(word, ")");
      break;
    }
    case SEQUENCE_COMMAND:
    {
      strcat(word, build_sys_string(c->u.command[0]));
      strcat(word, ";");
      strcat(word, build_sys_string(c->u.command[1]));
      break;
    }
    case AND_COMMAND:
    {
      strcat(word, build_sys_string(c->u.command[0]));
      strcat(word, "&&");
      strcat(word, build_sys_string(c->u.command[1]));
      break;
    }
    case OR_COMMAND:
    {
      strcat(word, build_sys_string(c->u.command[0]));
      strcat(word, "||");
      strcat(word, build_sys_string(c->u.command[1]));
      break;
    }
    case PIPE_COMMAND:
    {
      strcat(word, build_sys_string(c->u.command[0]));
      strcat(word, "|");
      strcat(word, build_sys_string(c->u.command[1]));
      break;
    }
  }
  return word;
}

//////////////////////////////////////////////////////////////////////////////
/*int execute (command_t c)
{
  int len = find_command_size(c);
  char* word = checked_malloc(len);
  
  switch (c->type)
  {
    case SIMPLE_COMMAND:
    {
      strcat(word, *(c->u.word));
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
      puts(*(c->u.command[0]->u.word));
      printf("len: %i\n", len);
      strcat(word, *(c->u.command[0]->u.word));
      strcat(word, "|");
      strcat(word, *(c->u.command[1]->u.word));
      c->status = system(word);
      printf("end of pipe execution\n");
      break;
    }
  }
  return c->status;
}
*/
void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  if (time_travel == 0)
    system(build_sys_string(c));
}
