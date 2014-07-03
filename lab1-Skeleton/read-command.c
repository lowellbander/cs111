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

typedef struct node *node_t;

struct command_stream 
{ 
  node_t curr;
};

struct node
{
  command_t self;
  node_t next;
};

char *join(const char* s1, const char* s2)
{
    char* result = checked_malloc(strlen(s1) + strlen(s2) + 1);

    strcpy(result, s1);
    strcat(result, s2);

    return result;
}

/* Checks to see that a character in the input stream is valid */
void validate(char* string, int line_num) {
  //TODO: finish implementation
  int i;
  int len = strlen(string);
  for (i = 0; i < len; ++i)
  {
    char c = string[i];
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
        c == '\n' ||
        c == '(' ||
        c == ')' ||
        c == ' ')
        continue;
      else
      {
        error(1, 0, "invalid character on line %i: %c", line_num, c);
        return;
      }
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
  //validate(c);

  while (c != EOF)
    {
      //validate(c);

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

  // begin syntax checking
  //while(ptr != beg)
  //{
  //  if (*ptr == '<')
  //    if (--ptr != beg && *ptr == '<')
  //      if (--ptr != beg && *ptr == '<')
  //        error (1, 0, "invalid syntax: <<<\n");
  //  --ptr;
  //}
  //ptr = end;
  //while(ptr != beg)
  //{
  //  if (*ptr == '>')
  //    if (--ptr != beg && *ptr == '>')
  //      if (--ptr != beg && *ptr == '>')
  //        error (1, 0, "invalid syntax: >>>n");
  //  --ptr;
  //}
  //ptr = end;
  // end syntax checking

  while(ptr != beg)
  {
    if (*ptr == ';')
      goto done;
    --ptr;
  }
  ptr = end;
  while(ptr != beg)
  {
    if (*ptr == ')')
      goto done;
    --ptr;
  }
  ptr = end;
  while(ptr != beg)
  { 
    if (*ptr == '|')
    {
      //Check if OR
      if(*(ptr-1) != '|')
        goto done;
      ptr--;
    }
    --ptr;
  }
  ptr = end;
  while(ptr != beg)
  {
    if (*ptr == '&')
      goto done;
    --ptr;
  }
  ptr = end;
  while(ptr != beg)
  {
    if (*ptr == '|')
      goto done;
    --ptr;
  }
  done:
  return ptr;
}

void check_syntax(char* beg, char* end, char* optPtr)
{
  //if (optPtr == beg)
  //  error (1, 0 , "bad syntax\n");
  return;
}

command_t
make_command (char* beg, char* end, int line_num)
{
  char* optPtr = get_opt_ptr(beg, end);
  check_syntax(beg, end, optPtr);
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
  if (optPtr == beg)
  {
    // check to see that operators have operands
    com->type = SIMPLE_COMMAND;
    com->u.word = checked_malloc(20*sizeof(char*));
    char* word = malloc(sizeof(char)*(end-beg));
    char* ptr = beg;
    int i = 0;
    for (; ptr != end + 1; ++ptr, ++i) 
    {
      word[i] = *ptr;
    }
    validate(word, line_num);
    *(com->u.word) = word;
  }
  else if (*optPtr == ';')
  {
    com->type = SEQUENCE_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 1, line_num);
    com->u.command[1] = make_command(optPtr + 1, end, line_num);
  }
  else if (*optPtr == ')')
  {
    com->type = SUBSHELL_COMMAND;
    char* open = beg;
    while (*open != '(') ++open;
    com->u.subshell_command = make_command(open + 1, optPtr - 1, line_num);
  }
  else if (*optPtr == '|' && *(optPtr-1) != '|')
  {
    com->type = PIPE_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 1, line_num);
    com->u.command[1] = make_command(optPtr + 1, end, line_num);
  }
  else if (*optPtr == '|')
  {
    com->type = OR_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 2, line_num);
    com->u.command[1] = make_command(optPtr + 1, end, line_num);
  }
  else if (*optPtr == '&')
  {
    com->type = AND_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 2, line_num);
    com->u.command[1] = make_command(optPtr + 1, end, line_num);
  }
  
  return com;
}

int line_nums(char* beg, char* end)
{
  int nLines = 0;
  char* ptr = beg;
  while (ptr != end)
  {
    if (*ptr == '\n')
      ++nLines;
    ++ptr;
  }
  return nLines;
}

void push(command_stream_t stream, command_t com)
{
  node_t node = checked_malloc(sizeof(struct node));
  node->self = com;
  node->next = NULL;

  if (stream->curr == NULL)
  {
    // the stream is empty 
    stream->curr = node;
    return;
  }
  else
  {
    // there are already nodes in the stream
    node_t last = stream->curr;
    while (last->next != NULL) last = last->next;
    last->next = node;
    return;
  }
}

char* copy(char* beg, char* end)
{
  int size = end - beg;
  char* string = checked_malloc(size);
  int i;
  for (i = 0; i < size; ++i, ++beg) 
    string[i] = *beg;

  return string;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  command_stream_t stream = checked_malloc(sizeof(struct command_stream));

  char* string = buildString(get_next_byte, get_next_byte_argument);
  char* end = string + strlen(string) - 2;
  
  int nLines = line_nums(string, end);
  if (nLines == 0) nLines = 1;
  int i;
  char* a = string;
  char* b;
  for (i = 0; i < nLines; ++i)
  {
      b = a;
      while (*b != '\n') ++b;
      char* line = copy(a, b);
      push(stream, make_command(line, line + (int)strlen(line), i+1));
      a = ++b;
  }
  
  return stream;
}

command_t
read_command_stream (command_stream_t s)
{
  if (s->curr == NULL)
    return NULL;
  else 
    {
      command_t com = s->curr->self;
      s->curr = s->curr->next;
      return com;
    }
}

