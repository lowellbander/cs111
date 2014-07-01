// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include <stdio.h>
#include <stdlib.h> 

#include <error.h>

struct command_stream 
{ 
  command_t head; 
  command_t curr;
};

/*
bool isValid(char c) {
  if (isalpha(c) || 
      isdigit(c) ||
      c == "!" ||
      c == "%" ||
      c == "+" ||
      c == "," ||
      c == "-" ||
      c == "." ||
      c == "/" ||
      c == ":" ||
      c == "@" ||
      c == "^" ||
      c == "_" )
      return true;
    else
      return false;
};
*/

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
  int size = INC;
  char* string = calloc(size, sizeof(char));
  int i = 0;
  char c  = get_next_byte(get_next_byte_argument);
  while (c != EOF)
    {
      if (i >= size)
        {
          string = resize(string, size + INC, size);
        }
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
  }
  return NULL;
}

command_t
make_command (char* beg, char* end)
{
  char* optPtr = get_opt_ptr(beg, end);
  //enum command_type typetest = PIPE_COMMAND;
  //struct command commie = {.type = typetest};
  
  //Check command type
  /*if (*optPtr == ')')
  {
    
  }
  else if (*optPtr == '|')
  {
    //If pipe
    if (*(--optPtr) != '|')
)   //insert making pipe command
    else
    //insert making OR command
  }
  else if (*optPtr == '&')
  //insert making AND command
  else
  //insert making simple command*/
  
  // TEMPORARY:
  command_t returnCommand = checked_malloc(sizeof(struct command));
  returnCommand->type = SIMPLE_COMMAND;
  returnCommand->u.word = checked_malloc(20*sizeof(char*));
  *(returnCommand->u.word) = "a>b<c";

  return returnCommand;
}

char* commandType(command_t com)
{
  enum command_type type = com->type;
  if (type == SIMPLE_COMMAND) return "type: SIMPLE_COMMAND\n";
  else return "command type not handled, yet\n";
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

  command_t com = make_command(string, end);

  command_stream_t stream = checked_malloc(sizeof(struct command_stream));

  stream->head = com;
  stream->curr = com;
  
  return stream;
}

command_t
read_command_stream (command_stream_t s)
{
  if (s->curr == s->head) printf("handling head\n");
  else printf("handling child\n");

  //printf(commandType(s->curr));

  command_t com = s->curr;

  s->curr = NULL;
  return com;

  error (1, 0, "command reading not yet implemented");
}
