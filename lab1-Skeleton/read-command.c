// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> 
#include <ctype.h>
#include <error.h>

struct command_stream 
{ 
  command_t head; 
  command_t curr;
};

/* Checks to see that a character in the input stream is valid */
void validate(char c) {
  //TODO: finish implementation
  if (isalpha(c) || 
      isdigit(c) ||
      c == '!' ||
      c == '%' ||
      c == '+' ||
      c == ',' ||
      c == '-' ||
      c == '.' ||
      c == '/' ||
      c == ':' ||
      c == '@' ||
      c == '^' ||
      c == '_' ||
      c == '<' ||
      c == '>' ||
      c == '\n' ||
      c == ' ')
      return;
    else
    {
      printf("invalid char: %c", c);
      error(1, 0, "invalid character");
      return;
    }
};

/* Builds a string from file stream */

//TODO: be sure it handles newline's well
char* buildString(int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  const int INC = 10;
  size_t size = INC;
  char* string = calloc(size, sizeof(char));
  int i = 0;

  char c  = get_next_byte(get_next_byte_argument);
  validate(c);

  while (c != EOF)
    {
      validate(c);

      if (strlen(string) >= size) 
        string = checked_grow_alloc(string, &size);

      string[i] = c;
      c  = get_next_byte(get_next_byte_argument);
      ++i;
    }

  return string;
}

/* Gets the pointer to relevant operator */

char* get_opt_ptr(char* beg, char* end)
{
  char* ptr = end;

  while(ptr != beg)
  {
    if (*ptr == '<')
      if (--ptr != beg && *ptr == '<')
        if (--ptr != beg && *ptr == '<')
          error (1, 0, "invalid syntax: <<<\n");
    --ptr;
  }
  ptr = end;
  while(ptr != beg)
  {
    if (*ptr == '>')
      if (--ptr != beg && *ptr == '>')
        if (--ptr != beg && *ptr == '>')
          error (1, 0, "invalid syntax: >>>n");
    --ptr;
  }
  ptr = end;

  while(ptr != beg)
  {
    if (*ptr == ';')
      return ptr;
    --ptr;
  }
  ptr = end;
  while(ptr != beg)
  {
    if (*ptr == ')')
      return ptr;
    --ptr;
  }
  ptr = end;
  while(ptr != beg)
  { 
    if (*ptr == '|')
    {
      //Check if OR
      if(*(ptr-1) != '|')
        return ptr;
      ptr--;
    }
    --ptr;
  }
  ptr = end;
  while(ptr != beg)
  {
    if (*ptr == '&')
      return ptr;
    --ptr;
  }
  ptr = end;
  while(ptr != beg)
  {
    if (*ptr == '|')
      return ptr;
    --ptr;
  }
  return NULL;
}

command_t
make_command (char* beg, char* end)
{
  char* optPtr = get_opt_ptr(beg, end);
  command_t com = checked_malloc(sizeof(struct command));
  
  /* OPERATOR PRECEDENCE
   * (highest)
   * ;
   * (, )
   * |
   * &&
   * ||
   * (lowest)
   * */

  //Check command type
  if (optPtr == NULL)
  {
    com->type = SIMPLE_COMMAND;
    com->u.word = checked_malloc(20*sizeof(char*));
    char* word = malloc(sizeof(char)*(end-beg));
    char* ptr = beg;
    int i = 0;
    for (; ptr != end + 1; ++ptr, ++i) 
    {
      word[i] = *ptr;
    }
    *(com->u.word) = word;
  }
  else if (*optPtr == ';')
  {
    com->type = SEQUENCE_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 1);
    com->u.command[1] = make_command(optPtr + 1, end);
  }
  else if (*optPtr == ')')
  {
    com->type = SUBSHELL_COMMAND;
    char* open = beg;
    while (*open != '(') ++open;
    com->u.subshell_command = make_command(open + 1, optPtr - 1);
  }
  else if (*optPtr == '|' && *(optPtr-1) != '|')
  {
    com->type = PIPE_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 1);
    com->u.command[1] = make_command(optPtr + 1, end);
  }
  else if (*optPtr == '|')
  {
    com->type = OR_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 2);
    com->u.command[1] = make_command(optPtr + 1, end);
  }
  else if (*optPtr == '&')
  {
    com->type = AND_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 2);
    com->u.command[1] = make_command(optPtr + 1, end);
  }
  
  return com;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{

  char* string = buildString(get_next_byte, get_next_byte_argument);
  char* end = string + strlen(string) - 2;

  command_t com = make_command(string, end);

  command_stream_t stream = checked_malloc(sizeof(struct command_stream));

  stream->head = com;
  stream->curr = com;
  
  return stream;
}

command_t
read_command_stream (command_stream_t s)
{
  command_t com = s->curr;
  s->curr = NULL;
  return com;
}

