// UCLA CS 111 Lab 1 command reading

/* 
  TODO: input/output
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
  char* string = checked_malloc(size+1);
  int i = 0;
  int numSpace = 0;

  while (beg != end)
  {
    if (*beg == ' ' || *beg == '\t')
      numSpace++;
    else
      numSpace = 0;

    // Skip extra white spaces in between
    if (numSpace <= 1)
    {
      string[i] = *beg;
      ++i;
    }

    ++beg;
  }
  //printf("copy: <");
  //puts(string);
  //printf(">\n");
  // End string properly
  string[i] = '\0';
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
  //puts(string);
  const char* end = string + len;
  int balance = 0;
  // Currently line is the last line number of the string
  int line = line_num;

  // Reset line number
  for (i = 0; i < len; ++i)
    if (string[i] == '\n') --line;

  for (i = 0; i < len; ++i)
  {
    bool left = false;
    bool right = false;
    char c = string[i];
    //printf("c: %c\n", c);
    //printf("line: %d\n", line);

    // Keep track of line number
    if (c == '\n')
      ++line;
    // Check for balanced parentheses
    if (c == '(') ++balance;
    if (c == ')') --balance;
    if (balance < 0) error(1, 0, "mismatching parentheses on line %d", line);

    // Check for invalid characters
    if (!isOperator(c) && !isOperand(c) && c != ' ' && c != '\t' && c != '\n')
      error(1, 0, "invalid character on line %i: [%c]", line, c);

    // Check for left and right operands
    // Cases: first & of AND, pipe, first | of OR, ;, <, >
    if ((c == '&' && string[i-1] != '&')||
        (c == '|' && string[i-1] != '|')|| 
         c == ';' ||
         c == '<' ||
         c == '>')
    { 
      //printf("Entering checking for left and right\n");
      // Check that it has left operands
      int l;
      for (l = i - 1; l != -1; --l)
      {
        if (isOperand(string[l])) 
        {
          left = true;
          break;
        }
        if (isOperator(string[l]))
        {
          left = false;
          error(1, 0, "found %c after %c without operand in between on line %d", c, string[l], line);
        }
      }
      if (!left) error(1, 0, "missing left operand for %c on line %i\n", c, line);
      // Check that it has right operands
      int r;
      // Skip over second & and | of AND and OR
      if (c == '&' || string[i+1] == '|') r = i + 2;
      else r = i + 1;
      for (; r < len; ++r)
      {
        if (isOperand(string[r])) 
        {
          right = true;
          break;
        }
        if (isOperator(string[r]))
        {
          right = false;
          error(1, 0, "found %c after %c without operand in between on line %d", string[r], c, line);
        }
      }
      if (!right) 
      {
        if (c == '&') 
          error(1, 0, "missing right operand for && on line %i\n", line);
        else if (c == '|' && string[i+1] == '|') 
          error(1, 0, "missing right operand for || on line %i\n", line);
        else
          error(1, 0, "missing right operand for %c on line %i\n", c, line);
      }
    }
  }

  if (balance != 0)
    error(1, 0, "unmatched parenthesis on line %i\n", line);
}

/* Builds a string from file stream */

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
  // End string properly
  if (strlen(string) >= size) 
      string = checked_grow_alloc(string, &size);
  string[i] = '\0';
  return string;
}

/* Gets the pointer to relevant operator */

char* get_opt_ptr(char* beg, char* end)
{
  // Make sure find operand before ; for sequence
  bool foundRand = false;
  char* seq = NULL;
  char* andor = NULL;
  char* pipe = NULL;
  char* paren = NULL; // closed paren )
  int paren_ctr = 0; // 0 => not inside parens
  char* ptr;
  for (ptr = end; ptr >= beg; --ptr)
  {
    char c = *ptr;
    if (paren_ctr == 0)
      switch (c) 
      {
        case ';':
          if (!seq && foundRand) seq = ptr;
          break;
        case '|':
        case '&':
          // Found first & or || not in parentheses
          if  ((*ptr == '&' && !andor) ||
               (*(ptr-1) == '|' && !andor))
            andor = ptr;
          // Found first pipe
          else if (*(ptr-1) != '|' && !pipe)
            pipe = ptr;          
          break;
        default:
          if (isOperand(c)) foundRand = true;
      }
    if (c == ')' && !paren) paren = ptr;
    if (c == ')') ++paren_ctr;
    if (c == '(') --paren_ctr;
  }
  if (seq) return seq;
  else if (andor) return andor;
  else if (pipe) return pipe;
  else if (paren) return paren;
  else return beg;
}

command_t
make_command (char* beg, char* end, int line_num)
{
  char* optPtr = get_opt_ptr(beg, end);
  command_t com = checked_malloc(sizeof(struct command));
  com->status = -1;   // command has not yet exited
  
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
    //printf("word: <"); puts(word); printf(">\n"); printf("length: %d\n", strlen(word));
    char* ptr;

    bool foundOperand = false;
    for (ptr = beg; ptr <= end; ++ptr)
    {
      //printf("Getting rid of leading\n");
      if (isOperand(*ptr) || isSpecial(*ptr)) 
      {
        foundOperand = true;
        beg = ptr;
        word = copy(beg, end);
        break;
      }
    }

    for (ptr = end; ptr >= beg; --ptr)
    {
      //printf("Getting rid of trailing\n");
      if (isOperand(*ptr) || isSpecial(*ptr)) 
      {
        end = ++ptr;
        word = copy(beg, end);
        break;
      }
      //else printf("skipiping over: <%c>\n", *ptr);
    }
    //printf("Making simple command: <");
    //puts(word);
    //printf(">\n");
    //if (!foundOperand)
    //  error(1, 0, "No operands before ; on line: %d\n", line_num);
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

  char* end = string + strlen(string) - 1;
    
  int nLines = line_nums(string, end);
  //int i = 0;
  char* a = string;
  char* b;

  char* beforeComment = a;
  int lineNum = 1;
  bool foundOp = false;
  bool finishedCommand = false;
  bool foundBegCommand = false;
  bool foundComment = false;
  b = a;
 
  while (b <= end)
  { 
    //printf("b: <%c>\n", *b);
/*if (foundOp) printf("foundOp = true\n"); else printf("foundOp = false\n");
if (finishedCommand) printf("finishedCommand = true\n"); else printf("finishedCommand = false\n");
if (foundBegCommand) printf("foundBegCommand = true\n"); else printf("foundBegCommand = false\n");
if (foundComment) printf("*******************foundComment = true\n"); else printf("foundComment = false\n");*/
     // Skip over comments
     if (isOperator(*b) && !foundBegCommand)
       error(1, 0, "Line %d may not start with operator", lineNum);
     if (*b == '#')
     {
       // If not first character
       if (b != string)
       {
         // Check if immediately preceded by ordinary token
         if (isalpha(*(b-1)) || isdigit(*(b-1)) ||
             *(b-1) == '!' ||
             *(b-1) == '%' ||
             *(b-1) == '+' ||
             *(b-1) == ',' ||
             *(b-1) == '-' ||
             *(b-1) == '.' ||
             *(b-1) == '/' ||
             *(b-1) == ':' ||
             *(b-1) == '@' ||
             *(b-1) == '^' ||
             *(b-1) == '_')
           error(1, 0, "invalid character # at line: %d\n", lineNum);
         else foundComment = true;
       }
       foundComment = true;
       beforeComment = b - 1;
     }
     //printf("b: <%c>\n", *b);
     else if (*b == '&')
     {
       // Check for beggining with operand
       if (finishedCommand && !foundComment)
         error(1, 0, "Cannot start with & on line: %d\n", lineNum);

       // Check to make sure not last character
       if (b == end && *(b-1) != '&')
         error(1, 0, "Missing second & on line: %d\n", lineNum);
       // Check for second &
       else if (*(b+1) != '&' && *(b-1) != '&')
         error(1, 0, "Missing second & on line: %d\n", lineNum);
       else if (!foundComment)
         foundOp = true;
     }
     else if (*b == '|')
     {
       // Check for beginnning with operand
       if (finishedCommand && !foundComment)
         error(1, 0, "Cannot start with | on line: %d\n", lineNum);
       else if (!foundComment)
         foundOp = true;
     }
     else if (*b == '\n')
     {
        ++lineNum;
        
        // Found end of complete command
        if (!foundOp && foundBegCommand && !foundComment)
        {
          //printf("Found end of complete command\n");
          char* tempString = checked_malloc(sizeof(copy(a, b+1)));
          tempString = copy(a, b+1);
          //puts(tempString);
          validate(tempString, lineNum);
          push(stream, make_command(tempString, 
                                    tempString + (int)strlen(tempString), 
                                    lineNum));
          a = b + 1;
          finishedCommand = true;
          foundOp = false;
          foundBegCommand = false;
        }
        else if (foundComment) 
        {
          // Found end of comment
          foundComment = false;
          // End of comment, make command preceding it
          if (foundBegCommand)
          {
            //printf("Found end of comment and making command!\n");
            char* tempString = checked_malloc(sizeof(copy(a, beforeComment+1)));
            tempString = copy(a, beforeComment+1);
            validate(tempString, lineNum);
            push(stream, make_command(tempString, 
                                      tempString + (int)strlen(tempString), 
                                      lineNum));
            a = b + 1;
            finishedCommand = true;
            foundOp = false;
            foundBegCommand = false;
          }
          // Comment came after already made complete command
          else a = b;
        }
     }
     else if (*b != ' ' && *b != '\t' && !foundComment)
     {
       //printf("foundbegcommand\n");
       finishedCommand = false;
       foundOp = false;
       foundBegCommand = true;
     }
     ++b;
  }
  
  // Check for unfinished commands
  if (!finishedCommand)
  {
    char* tempString;
    //printf("Finishing command:\n");
    tempString = copy(a, b+1);
    //puts(tempString);
    validate(tempString, lineNum);
    push(stream, make_command(tempString,
                              tempString + (int)strlen(tempString),
                              lineNum));
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

