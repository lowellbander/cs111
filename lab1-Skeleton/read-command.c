// UCLA CS 111 Lab 1 command reading

/* TODO:comments
        make_command -- OR, AND
        deal with new line counts, passing it back up tree
        syntax error messages
        read_command

*/ 

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
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
bool isValid(char c) 
{
  //TODO: finish implementation
  //return true;
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
      c == ' ' ||
      c == '\n'||
      c == '\t')
      return true;
    else
      return false;
};


/* Used for reallocating space for char string */
//TODO: replace this with realloc()
char* resize(char* old, int newsize, int oldsize) 
{
  char* new = calloc(newsize, sizeof(char));
  int i = 0;
  for (i = 0; i < oldsize; ++i)
  {
    new[i] = old[i];
  }
  return new;
}

/* Builds a string from file stream */

//TODO: be sure it handles newline's well
char* buildString(int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  const int INC = 10;
  size_t size = INC;
  char* string = calloc(size, sizeof(char));
  int i = 0;
  int line = 1;
  char c  = get_next_byte(get_next_byte_argument);
  if(c == '\n')
    ++line;
  while (c != EOF)
  {
    if (!isValid(c)) 
      error (1, 0, "Syntax error, invalid <%c>: Line: %d", c, line);
    if ((size_t)i >= size)
    {
      size += INC;
      string = checked_grow_alloc(string, &size);
    }
    string[i] = c;
    c  = get_next_byte(get_next_byte_argument);
    if(c == '\n')
      ++line;
    ++i;
  }
  return string;
}

/* Gets the pointer to relevant operator */

char* get_opt_ptr(char* beg, char* end)
{
  char* ptr = end;
  // Scan for sequence command
  while(ptr != beg)
  {
    if (*ptr == ';')
      return ptr;
    --ptr;
  }
  ptr = end;
  // Scan for subshell command
  while(ptr != beg)
  {
    if (*ptr == ')')
      return ptr;
    --ptr;
  }
  ptr = end;
  // Scan for pipe command
  while(ptr != beg)
  { 
    if (*ptr == '|')
    {
      // Check if pipe
      if(*(ptr-1) != '|')
        return ptr;
      ptr--;
    }
    --ptr;
  }
  ptr = end;
  // Scan for AND command
  while(ptr != beg)
  {
    if (*ptr == '&')
      return ptr;
    --ptr;
  }
  ptr = end;
  // Scan for OR command
  while(ptr != beg)
  {
    if (*ptr == '|')
      return ptr;
    --ptr;
  }
  return NULL;
}

/* Makes a single command, can be called recursively */

command_t
make_command (char* beg, char* end, int line)
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

  // Check command type and makes command, calling function recursively if needed
  if (optPtr == NULL)
  {
    com->type = SIMPLE_COMMAND;
    com->u.word = checked_malloc(20*sizeof(char*));
    char* word = checked_malloc(sizeof(char)*(end-beg));
    char* ptr = beg;
    int i = 0;
    bool foundSpace = false;
    for (; ptr != end + 1; ++ptr) 
    {  
      // Check if newline
      if (*ptr == '\n')
      {
        ++line;
        foundSpace = false;
      }
      else
      {
        // Skip over extra white spaces and tabs
        if (!foundSpace && *ptr != '\t')
        {
          word[i] = *ptr;
          if (*ptr == ' ')
            foundSpace = true;
          else
            foundSpace = false;
          ++i;
        }
        else
        {
          if (*ptr != ' ' && *ptr != '\t')
          {
            word[i] = *ptr;
            ++i;
            foundSpace = false;
          }
          else if (*ptr != '\t')
            foundSpace = true;
        }
      }
    }
    *(com->u.word) = word;
  }
  else if (*optPtr == ';')
  {
    com->type = SEQUENCE_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 1, line);
    com->u.command[1] = make_command(optPtr + 1, end, line);
  }
  else if (*optPtr == ')')
  {
    com->type = SUBSHELL_COMMAND;
    char* open = beg;
    while (*open != '(') ++open;
    com->u.subshell_command = make_command(open + 1, optPtr - 1, line);
  }
  else if (*optPtr == '|' && *(optPtr-1) != '|')
  {
    com->type = PIPE_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 1, line);
    com->u.command[1] = make_command(optPtr + 1, end, line);
  }
  else if (*optPtr == '|')
  {
    com->type = OR_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 1, line);
    com->u.command[1] = make_command(optPtr + 1, end, line);
  }
  else
  {
    if (*optPtr == '&' && *(optPtr-1) != '&')
      error (1, 0, "Syntax error, no second &: Line: %d", line);
    com->type = AND_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 1, line);
    com->u.command[1] = make_command(optPtr + 1, end, line);
  }
  
  return com;
  
}

char* commandType(command_t com)
{
  enum command_type type = com->type;
  if (type == SIMPLE_COMMAND) 
    return "type: SIMPLE_COMMAND\n";
  else 
    return "command type not handled, yet\n";
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  char* string = buildString(get_next_byte, get_next_byte_argument);
  //puts(string);

  // Determine the size of the string
  int size = 0;
  while (string[size] != 0) { ++size; }
  --size;
  char* end = string + size -1;

  command_t com = make_command(string, end, 0);

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

