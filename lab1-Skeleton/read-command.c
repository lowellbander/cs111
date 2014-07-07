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

char* copy(char* beg, char* end)
{
  int size = end - beg;
  char* string = checked_malloc(size);
  int i;
  for (i = 0; i < size; ++i, ++beg) 
    string[i] = *beg;
  puts(string);
  return string;
}

bool isOperator(char c)
{
  if (c == '|' || c == '&' || c == ';' || c == '<' || c == '>')
    return true;
  else
    return false;
}

bool isSpecial(char c) 
{
  if (c == '(' ||
      c == ')' ||
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
      c == '_' )
      return true;
    else return false;
}

bool isOperand(char c) 
{
  if (isalpha(c) || isdigit(c) || isSpecial(c)) return true;
  else return false;
}

/* Checks to see that a line in the input stream is valid */
void validate(char* string, int line_num) {
  int i;
  int len = strlen(string);
  //printf("length: %d\n", len);
  
  int balance = 0;
  for (i = 0; i < len; ++i)
  {
    if (string[i] == ')') ++balance;
    if (string[i] == '(') --balance;
  }
  if (balance != 0)
    error(1, 0, "unmatched parenthesis on line %i\n", line_num);
  
  //check for invalid characters
  for (i = 0; i < len; ++i)
  {
    char c = string[i];

    if (isOperator(c) || isOperand(c) || c == ' ' || c == '\t' || c == '\n') continue;
    else error(1, 0, "invalid character on line %i: %c", line_num, c);
  }

  //check for double/triple operators
  char* ops = "<>|&;";
  for (i = 0; i < (int)strlen(ops); ++i)
  {
    char op = ops[i];
    char* ptr;
    const char* end = string + len;
    for (ptr = string; ptr < end; ++ptr)
    {
      if (*ptr == op)
        if (++ptr != end && *ptr == op)
        {
          if (op == ';')
            error(1, 0, "invalid sequence on line %i: %c%c%c\n", 
                                                 line_num, op, op, op);
          else if (++ptr != end && *ptr == op)
            error(1, 0, "invalid sequence on line %i: %c%c%c\n", 
                                                 line_num, op, op, op);
        }
    }
  }

  //check that all operators have operands
  for (i = 0; i < len; ++i)
  {
    bool left = false;
    char c = string[i];
    if (isOperator(c))
    {
      if ((c == '&' && string[i+1] == '&') || (c == '|' && string[i+1] == '|'))
        ++i;

      //check that it has left and right operands
      int j;
      //left
      for (j = i - 2; j != -1; --j)
      {
        if (isOperand(string[j])) 
        {
          left = true;
          break;
        }
        if (isOperator(string[j]))
        {
          left = false;
          break;
        }
      }

      if (!left) error(1, 0, "missing left operand for %c on line %i\n", 
                                                    c, line_num);
    }
  }
  for (i = len-1; i > -1; --i)
  {
    bool right = false;
    char c = string[i];
    if (isOperator(c))
    {
      if ((c == '&' && string[i-1] == '&') || (c == '|' && string[i-1] == '|'))
        --i;
      //check that it has left and right operands
      int j;
      //left
      for (j = i + 2; j < len; ++j)
      {
        if (isOperand(string[j])) 
        {
          right = true;
          break;
        }
        if (isOperator(string[j]))
        {
          right = false;
          break;
        }
      }

      if (!right) error(1, 0, "missing right operand for %c on line %i\n", 
                                                    c, line_num);
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

  while (c != EOF)
  {
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

command_t
make_command (char* beg, char* end, int line_num)
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
  if (optPtr == beg)
  {
    //TODO: deprecate
    if (isOperator(*optPtr)) 
      error(1, 0, "too few operands on line %i\n", line_num);

    com->type = SIMPLE_COMMAND;
    com->u.word = checked_malloc(20*sizeof(char*));
    char* word = copy(beg, end);
    char* ptr;

    //check for leading white space
    for (ptr = beg; ptr <= end; ++ptr)
    {
      if (isOperand(*ptr) || isSpecial(*ptr)) 
      {
        beg = ptr;
        word = copy(beg, end);
        break;
      }
    }
    //check for trailing white space
    for (ptr = end; ptr >= beg; --ptr)
    {
      if (isOperand(*ptr) || isSpecial(*ptr)) 
      {
        end = ++ptr;
        word = copy(beg, end);
        break;
      }
    }
    printf("Making simple command: <");
    puts(word);
    printf(">\n");
    *(com->u.word) = word;
  }
  else if (*optPtr == ';')
  {
    //printf("Making sequence command\n");
    com->type = SEQUENCE_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 1, line_num);
    com->u.command[1] = make_command(optPtr + 1, end, line_num);
  }
  else if (*optPtr == ')')
  {
    //printf("Making subshell command\n");
    com->type = SUBSHELL_COMMAND;
    char* open = beg;
    while (*open != '(') ++open;
    com->u.subshell_command = make_command(open + 1, optPtr - 1, line_num);
  }
  else if (*optPtr == '|' && *(optPtr-1) != '|')
  {
    //printf("Making pipe command\n");
    com->type = PIPE_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 1, line_num);
    com->u.command[1] = make_command(optPtr + 1, end, line_num);
  }
  else if (*optPtr == '|')
  {
    //printf("Making OR command\n");
    com->type = OR_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 2, line_num);
    com->u.command[1] = make_command(optPtr + 1, end, line_num);
  }
  else if (*optPtr == '&')
  {
    //printf("Making AND command\n");
    com->type = AND_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 2, line_num);
    com->u.command[1] = make_command(optPtr + 1, end, line_num);
  }
  
  return com;
}

int line_nums(char* beg, char* end)
{
  int nLines = 1;
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

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  command_stream_t stream = checked_malloc(sizeof(struct command_stream));

  char* string = buildString(get_next_byte, get_next_byte_argument);

  //char* end = string + strlen(string) - 2;
  char* end = string + strlen(string) - 1;
    
  int nLines = line_nums(string, end);
  int i;
  char* a = string;
  char* b;

  i = 0;
  int lineNum = 0;
  bool foundOp = false;
  bool finishedCommand = false;
  bool foundBegCommand = false;
  b = a;
  // End doesn't seem to be calculated right 
  //++end;
  while (b != end)
  {
     //printf("b: <%c>\n", *b);
     if (*b == '&')
     {
       // Check for beggining with operand
       if (finishedCommand)
         error(1, 0, "Cannot start with & on line: %d\n", lineNum);

       // Check to make sure not last character
       if (i == (int)(strlen(string) - 1))
         error(1, 0, "Missing second & on line: %d\n", lineNum);
       // Check for second &
       else if (*(b+1) != '&')
         error(1, 0, "Missing second & on line: %d\n", lineNum);
       else
         foundOp = true;
     }
     else if (*b == '|')
     {
       // Check for beginnning with operand
       if (finishedCommand)
         error(1, 0, "Cannot start with | on line: %d\n", lineNum);
       else
         foundOp = true;
   
     }
     else if (*b == '\n')
     {
        ++lineNum;
        
        // Found end of complete command
        if (!foundOp && foundBegCommand)
        {
          //printf("Found end of complete command\n");
          char* tempString = copy(a, b+1);
          validate(tempString, lineNum);
          push(stream, make_command(tempString, 
                                    tempString + (int)strlen(tempString), 
                                    lineNum));
          a = b + 1;
          finishedCommand = true;
          foundOp = false;
          foundBegCommand = false;
        }
     }
     else if (*b != ' ' && *b != '\t')
     {
       finishedCommand = false;
       foundOp = false;
       foundBegCommand = true;
     }
     ++b;
  }
  
  // Check for unfinished commands
  if (!finishedCommand)
  {
    //printf("Finishing command:\n");
    char* tempString = copy(a, b+1);
    validate(tempString, lineNum);
    push(stream, make_command(tempString,
                              tempString + (int)strlen(tempString),
                              lineNum));
  }

  // Lowell's, also changed end to -1 instead of -2
  /*for (i = 0; i < nLines; ++i)
  {
      b = a;
      while (*b != '\n') ++b;
      char* line = copy(a, b);
      validate(line, i+1);
      push(stream, make_command(line, line + (int)strlen(line), i+1));
      a = ++b;
  }*/
  
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

