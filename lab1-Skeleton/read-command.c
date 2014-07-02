// UCLA CS 111 Lab 1 command reading


/* TODO:comments
        syntax checker
        make_command_stream --> recognize separate commands 
        deal with new line counts, passing it back up tree for syntax errors
        read_command --> trivial, but make commands a linked list
        
*/ 


/*For Nicole: change make_command's arguments to include line number
              command stream
  
*/

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
  //TODO: finish implementation and add argument for line number
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
      c == '|' ||
      c == ';' ||
      c == '&' ||
      c == '(' ||
      c == ')' ||
      c == ' ' ||
      c == '\n' ||
      c == '\t')
      return;
    else
    {
      error(1, 0, "invalid character: %c", c);
      return;
    }
};

/* Builds a string from file stream */

//TODO: be sure it handles newline's well
char* buildString(int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  size_t size = 8;
  char* string = checked_malloc(8*sizeof(char));
  int i = 0;
  int line = 1;

  char c  = get_next_byte(get_next_byte_argument);
  validate(c);

  if(c == '\n')
    ++line;
  while (c != EOF)
    {
      validate(c);

      if (strlen(string) >= size) 
        string = checked_grow_alloc(string, &size);
      string[i] = c;
      c  = get_next_byte(get_next_byte_argument);
      if(c == '\n')
        ++line;
      ++i;
    }
  //Null terminate string
  string[i] = '\0';
  return string;
}

/* Gets the pointer to relevant operator */

char* get_opt_ptr(char* beg, char* end)
{
  char* ptr = end;

  // begin syntax checking
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
  // end syntax checking

  // Scan for sequence command
  while(ptr != beg)
  {
    if (*ptr == ';')
      goto done;
    --ptr;
  }
  ptr = end;
  // Scan for subshell command
  while(ptr != beg)
  {
    if (*ptr == ')')
      goto done;
    --ptr;
  }
  ptr = end;
  // Scan for pipe command
  while(ptr != beg)
  { 
    if (*ptr == '|')
    {
      //Check if pipe
      if(*(ptr-1) != '|')
        goto done;
      ptr--;
    }
    --ptr;
  }
  ptr = end;
  // Scan for AND command
  while(ptr != beg)
  {
    if (*ptr == '&')
      goto done;
    --ptr;
  }
  ptr = end;
  // Scan for OR command
  while(ptr != beg)
  {
    if (*ptr == '|')
      goto done;
    --ptr;
  }
  done:
  return ptr;
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

  // Check command type and makes command, calling function recursively if needed
  if (optPtr == beg)
  {
    // check to see that operators have operands


    com->type = SIMPLE_COMMAND;
    com->u.word = checked_malloc(20*sizeof(char*));
    char* word = malloc(sizeof(char)*(end-beg));
    char* ptr = beg;
    int i = 0;
    int line = 0;
    bool foundSpace = false;

    for (; ptr != end; ++ptr) 
    {  
      printf("i: %d, c: <%c>\n", i, *ptr);
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
          //printf("Writing1\n");
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
            //printf("Writing2\n");
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
  char* end = string + strlen(string);
  printf("strlen: %d  end: <%c>\n", strlen(string), *end);
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

