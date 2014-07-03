// UCLA CS 111 Lab 1 command reading


/* TODO:comments
        syntax checker
        deal with new line counts, passing it back up tree for syntax errors
        read_command
        input/output
        
*/ 


/*Nicole: change make_command's arguments to include line number
              command stream
                --can simple commands ever have new lines? what about after +?
                --can simple commands start/end with non alpha/digit?
                --check for semicolons at the end of simple command
              read command stream --- infinitely loops still
  
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
  command_t command;
  command_stream_t prev; 
  command_stream_t next;
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
      {
        size += size;
        string = checked_grow_alloc(string, &size);
      }
      string[i] = c;
      c  = get_next_byte(get_next_byte_argument);
      if(c == '\n')
        ++line;
      ++i;
    }
  //Null terminate string
  size += 1;
  if (strlen(string) >= size) 
    string = checked_grow_alloc(string, &size);
  string[i] = '\0';
  return string;
}

/* Gets the pointer to relevant operator */

char* get_opt_ptr(char* beg, char* end)
{
  char* ptr = end;

  // begin syntax checking
  // TODO: Check for < < < or > > > or > > or < < or < + or + <
  bool foundNonWhite = false;
  while(ptr != beg)
  {
    // Checking for <<<
    if (*ptr == '<')
      if (--ptr != beg && *ptr == '<')
        if (--ptr != beg && *ptr == '<')
          error (1, 0, "invalid syntax: <<<\n");
    --ptr;
  }
  ptr = end;
  // Checking for >>>
  while(ptr != beg)
  {
    if (*ptr == '>')
      if (--ptr != beg && *ptr == '>')
        if (--ptr != beg && *ptr == '>')
          error (1, 0, "invalid syntax: >>>\n");
    --ptr;
  }
  ptr = end;
  // end syntax checking

  // Scan for sequence command
  while(ptr != beg)
  {
    // Checking for sequence command or end of complete command
    if (!foundNonWhite && *ptr != '\n' 
                       && *ptr != '\t' 
                       && *ptr != '\n' 
                       && *ptr != EOF 
                       && *ptr != '\0' 
                       && *ptr != ';')
    {
      foundNonWhite = true;
    }
    if (foundNonWhite && *ptr == ';')
    {
      printf("It's a sequence command for sure!\n");
      goto done;
    }
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
  // Begin making simple command
  if (optPtr == beg)
  {
    // check to see that operators have operands

    //TODO: Check for ; at the end of simple command
    com->type = SIMPLE_COMMAND;
    com->u.word = checked_malloc(20*sizeof(char*));
    char* word = malloc(sizeof(char)*(end-beg+1));
    char* ptr = beg;
    int i = 0;
    int line = 0;
    // Consecutive white spaces
    int consecSpace = 0;;
    bool foundBegWord = false;

    // Create simple command, skipping over white space
    for (; ptr != end; ++ptr) 
    {  
      //Find start of simple command
      if (!foundBegWord && *ptr != ' ' && *ptr != '\t')
      {
        // Syntax checking for simple command -- check it starts with digit/alpha
        if (!isalpha(*beg) && !isdigit(*beg) && *beg != ' ' && *beg != '\t' && *beg != '\n')
          error(1, 0, "Syntax error: Line: %d, simple command cannot start with <%c>\n", line, *beg);
        word[i] = *ptr;
        ++i;
        foundBegWord = true;
        consecSpace = 0;
      }
      // Skip over extra white spaces/tabs
      else
      { 
        if (consecSpace == 0 || (*ptr != ' ' && *ptr != '\t'))
        {
          word[i] = *ptr;
          ++i;
        }
        if (*ptr == ' ' || *ptr == '\t')
          ++consecSpace;
        else
          consecSpace = 0;
      }
    }
    word[i] = '\0';
    // Syntax check for end semi colon and get rid of it
    if (word[i-1] == ';')
      word[i-1] = '\0';
    *(com->u.word) = word;
    printf("Made simple command: ");
    puts(word);
  }
  // Begin making sequence command
  else if (*optPtr == ';')
  {
    printf("Making sequence command\n");
    com->type = SEQUENCE_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 1);
    com->u.command[1] = make_command(optPtr + 1, end);
  }
  // Begin making subshell command
  else if (*optPtr == ')')
  {
    printf("Making subshell command\n");
    com->type = SUBSHELL_COMMAND;
    char* open = beg;
    while (*open != '(') ++open;
    com->u.subshell_command = make_command(open + 1, optPtr - 1);
  }
  // Begin making pipe command
  else if (*optPtr == '|' && *(optPtr-1) != '|')
  {
    printf("Making pipe command\n");
    com->type = PIPE_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 1);
    com->u.command[1] = make_command(optPtr + 1, end);
  }
  // Begin making OR command
  else if (*optPtr == '|')
  {
    printf("Making OR command\n");
    com->type = OR_COMMAND;
    com->u.command[0] = make_command(beg, optPtr - 2);
    com->u.command[1] = make_command(optPtr + 1, end);
  }
  // Begin making AND command
  else if (*optPtr == '&')
  {
    printf("Making AND command\n");
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
  command_stream_t headStream = checked_malloc(sizeof(struct command_stream));
  command_stream_t currStream = checked_malloc(sizeof(struct command_stream));
  //Set current stream pointer to head
  currStream = headStream;
  headStream->next = NULL;
  headStream->prev = NULL;
  char* string = buildString(get_next_byte, get_next_byte_argument);
  char* beg = string;
  int stringSize = strlen(string);
  char* end = string + stringSize;

  // begin find separate complete commands

  // Keep track whether operator was found last until command (get rid of whitespace)
  bool foundOperator = false;
  // Keep track of whether found beginning of complete comman
  int cmdSize = 0;
  // Keep track if head command was found
  bool foundHead = false;
  int i = 0;
  printf("Size: %d\n", stringSize);
  for (i = 0; i <= stringSize; ++i)
  { 
    printf("i: %d, string[%d]: <%c>\n", i, i, string[i]);
    if (cmdSize > 0) printf("Found cmd beg\n");
    if (foundHead) printf("Found head\n");
    if (foundOperator) printf("Found op\n");
    // Check for operator waiting for operand
    if (string[i] == '|' || string[i] == '&' || string[i] == '(')
    {
      if (string[i] == '(')
        cmdSize += 1;
      foundOperator = true;
    }
    // Found new line to signal end of command 
    else if (!foundOperator && cmdSize > 0 && string[i] == '\n')
    {
      printf("Trying to make command\n");
      // If found head, create another one in list to fill in
      if (foundHead)
      {
        printf("Found head and making a command\n");
        // Create another one in list and point to next one in stream
        command_stream_t nextStream = checked_malloc(sizeof(struct command_stream));
        currStream->next = nextStream;
        nextStream->command = make_command(beg, &string[i]);
        nextStream->prev = currStream;
        nextStream->next = NULL;
        currStream = nextStream;
      }
      else
        currStream->command = make_command(beg, &string[i]);
      beg = &string[i];
      cmdSize = 0;
      foundHead = true;
    }
    // If found operator, check if whitespace or operand
    else if (foundOperator && string[i] != ' ' && string[i] != '\n' && string[i] != '\t')
      foundOperator = false;
    else if (string[i] != ' ' && string[i] != '\n' && string[i] != '\t')
    {
      // Found beginning of command
      if (cmdSize == 0)
        beg = &string[i];

      cmdSize += 1;
    }
  }
  //Check to see if whole script was one command or if there are any remaining commands left
  if (!foundHead)
  {
    printf("Never found head, making command.\n");
    currStream->command = make_command(beg, end);
  }
  else if (cmdSize > 0)
  {
    printf("Making last command\n");
    command_stream_t nextStream = checked_malloc(sizeof(struct command_stream));
    currStream->next = nextStream;
    nextStream->command = make_command(beg, end);
    nextStream->prev = currStream;
    nextStream->next = NULL;
    currStream = nextStream;
  }

  if (currStream->next == NULL && currStream->prev == headStream && headStream->next == currStream)
    printf("Passed Sanity Check\n");
  
  return headStream;
}


// TODO: figure out why if s->next is NULL, setting s = s->next does not make s NULL upon leaving function
//         currently infinitely loops, thinking s->next is never NULL

command_t
read_command_stream (command_stream_t s)
{
  printf("entering read command\n");
  //s = s->next;
  if (s == NULL)
    printf("Stream is indeedy NULL\n");
  else
    printf(".....\n");
  if (s->next == NULL)
    printf("Next is NULL\n");
  else
    printf("oh crapola\n");
  command_t com = checked_malloc(sizeof(struct command));
  com = s->command;
  if (s == NULL)
    return NULL;
  else
  {
    printf("Moving on to next\n");
    printf("s: %p\n", s);
    printf("n: %p\n", s->next);
    s = s->next;
    printf("nexts: %p\n", s);
    //printf("nextn: %p\n", s->next);
  }
  return com;
}

