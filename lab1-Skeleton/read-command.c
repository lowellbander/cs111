// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include <stdio.h>
#include <stdlib.h> 

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

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

char* stretch(char* old, int newsize, int oldsize) 
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
          string = stretch(string, size + INC, size);
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
  
  command_t returnCommand = malloc(sizeof(struct command));
  returnCommand->type = SIMPLE_COMMAND;
  char* foo = "asdg";
  returnCommand->u.word = &foo;
  return returnCommand;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */

  char* string = buildString(get_next_byte, get_next_byte_argument);
  puts(string);

  int size = 0;
  while (string[size] != 0)
  {
    ++size;
  }
  --size;
  char* end = string + size -1;
  command_t com = make_command(string, end);
  command_stream_t stream = malloc(sizeof(struct command_stream));
  stream->head = com;
  stream->curr = com;
  
  return stream;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  if (s->curr == NULL) return 0;
  command_t temp = s->curr;
  s->curr = NULL;
  printf("asd: <%s>", *(temp->u.word));
  return temp;

  
  error (1, 0, "command reading not yet implemented");
  return 0;
}
