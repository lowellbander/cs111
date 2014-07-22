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

/* Creates new string from old without 
   leading, extra, or trailing white spaces */

char* delete_white(char* string)
{
  const int INC = 10;
  size_t size = INC;
  char* new_string = checked_malloc(8*sizeof(char));
  int len = strlen(string);
  bool found_beg = false;
  int consec_space = 0;
  // string's counter
  int i;
  // new_string's counter
  int n = 0;
  for (i = 0; i < len; ++i)
  {
    char c = string[i];
    // Get rid of leading
    if (c != ' ' && c != '\t' && c != '\n')
    {
      found_beg = true;
      consec_space = 0;
      if (strlen(new_string) >= size) 
        string = checked_grow_alloc(string, &size);
       new_string[n] = c;
       ++n;
    }
    else if (found_beg && (c == ' ' || c== '\t' || c == '\n'))
    {
      ++consec_space;
      // Skip over extra white spaces
      if (consec_space == 1)
      {
        if (strlen(new_string) >= size) 
          string = checked_grow_alloc(string, &size);
        new_string[n] = ' ';
        ++n;
      }
    } 
  }
  
  // Null terminate string
  // Check if last inputted character is white space
  if (new_string[n-1] == ' ')
  {
    new_string[n-1] = '\0';
  }
  else
  {
    if (strlen(new_string) >= size) 
      string = checked_grow_alloc(string, &size);
    new_string[n] = '\0';
    ++n;
  }
  return new_string;
}

char* copy(char* beg, char* end)
{
  int size = end - beg;
  char* string = checked_malloc(size+1);
  int i = 0;
  int num_space = 0;

  while (beg != end)
  {
    if (*beg == ' ' || *beg == '\t')
      num_space++;
    else
      num_space = 0;

    // Skip extra white spaces in between
    if (num_space <= 1)
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

bool is_operator(char c)
{
  if (c == '|' || c == '&' || c == ';' || c == '<' || c == '>')
    return true;
  else
    return false;
}

bool is_special(char c) 
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

bool is_operand(char c) 
{
  if (isalpha(c) || isdigit(c) || is_special(c)) return true;
  else return false;
}

/* Checks to see that a line in the input stream is valid */
void validate(char* string, int line_num) {
  int i;
  int len = strlen(string);
  //printf("length: %d\n", len);
  const char* end = string + len;
  int balance = 0;
  // Currently line is the last line number of the string
  int line = line_num;

  // Reset line number
  for (i = 0; i < len; ++i)
    if (string[i] == '\n') --line;

  bool found_arrow = false;
  for (i = 0; i < len; ++i)
  {
    bool left = false;
    bool right = false;
    char c = string[i];
    //printf("c: [%c]\n", c);
    //printf("line: %d\n", line);

    // Keep track of line number
    if (c == '\n')
      ++line;
    // Check for balanced parentheses
    if (c == '(') ++balance;
    if (c == ')') --balance;
    if (balance < 0) error(1, 0, "mismatching parentheses on line %d", line);

    // Check for invalid characters
    if (!is_operator(c) && !is_operand(c) && c != ' ' && c != '\t' && c != '\n')
      error(1, 0, "invalid character on line %i: [%c]", line, c);

    // Check for left and right operands for > and <
    // Valid cases: a<b>c, d<e, f>g 
    // Invalid cases: a>b<c, a<b<c, a>b>c
    if (c == '<' || c == '>')
    {
      //printf("Found arrow: %c\n", c);
      // Whether beginning of operand was found
      int l;
      // Check for left operand
      for (l = i - 1; l != -1; --l)
      {
        //printf("****************\nstring[l]: [%c]\n", string[l]);
        //if (left) printf("left = true\n"); else printf("left = false\n");

        if (is_operand(string[l])) 
          left = true;
        else if (is_operator(string[l]) && !left)
          error(1, 0, "found %c after %c without operand in between on line: %d", c, string[l], line);
        // Break once find beg of simple command
        else if (left && ((is_operator(string[l]) && 
                          string[l] != '<' && 
                          string[l] != '>') || string[l] == '\n')) break;
  
        // Check for a<b<c, a>b>c, a>b<c
        if (string[l] == '>' || (string[l] == '<' && c == '<'))
          error(1, 0, "can not have %c before %c on line: %d", string[l], c, line);
        // a<b>c is okay
        else if (string[l] == '<' && c == '>') break;
      }

      if (!left)
        error(1, 0, "missing left operand for %c on line %d", c, line);
      int r;
      // Check for right operand
      for (r = i + 1; r < len; ++r)
      {
        if (is_operand(string[r])) 
          right = true;
     
        if (is_operator(string[r]) && !right)
          error(1, 0, "found %c after %c without operand in between on line: %d", string[r], c, line);
        
        // found end of simple command
        if (right && (is_operator(string[r]) || string[r] == '\n')) break;
      }
    }

    // Check for left and right operands
    // Cases: first & of AND, pipe, first | of OR, ;
    if ((c == '&' && string[i-1] != '&')||
        (c == '|' && string[i-1] != '|')|| 
         c == ';')
    { 
      left = false;
      right = false;
      //printf("Entering checking for left and right\n");
      // Check that it has left operands
      int l;
      for (l = i - 1; l != -1; --l)
      {
        if (is_operand(string[l])) 
        {
          left = true;
          break;
        }
        if (is_operator(string[l]))
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
        if (is_operand(string[r])) 
        {
          right = true;
          break;
        }
        if (is_operator(string[r]))
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
  // Check parenthesis balance
  if (balance != 0)
    error(1, 0, "unmatched parenthesis on line %i\n", line);
}

/* Builds a string from file stream */

char* build_string(int (*get_next_byte) (void *),
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

/* Given the beginning and end of string,
   return the pointer to relevant operator */

char* get_opt_ptr(char* beg, char* end)
{
  // Make sure find operand before ; for sequence
  bool found_operand = false;
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
    {
      switch (c) 
      {
        case ';':
          if (!seq && found_operand) seq = ptr;
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
          if (is_operand(c)) found_operand = true;
      }
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
    if (is_operator(*optPtr)) 
      error(1, 0, "too few operands on line %i\n", line_num);

    com->type = SIMPLE_COMMAND;
    com->u.word = checked_malloc(20*sizeof(char*));
    char* word = copy(beg, end);
    //printf("word: <"); puts(word); printf(">\n"); printf("length: %d\n", strlen(word));
    char* ptr;

    bool found_operand = false;
    for (ptr = beg; ptr <= end; ++ptr)
    {
      //printf("Getting rid of leading\n");
      if (is_operand(*ptr) || is_special(*ptr)) 
      {
        found_operand = true;
        beg = ptr;
        word = copy(beg, end);
        break;
      }
    }

    for (ptr = end; ptr >= beg; --ptr)
    {
      //printf("Getting rid of trailing\n");
      if (is_operand(*ptr) || is_special(*ptr)) 
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
    //if (!found_operand)
    //  error(1, 0, "No operands before ; on line: %d\n", line_num);
    *(com->u.word) = word;
  
    // Separate into input and output
    char* the_word = NULL;
    char* input = NULL;
    char* output = NULL;
    // Found > or <
    bool found_arrow = false;
    // Found beginning of input or output
    bool found_io = false;
    for (ptr = beg; ptr <= end; ++ptr)
    {
      //printf("****************\nbeg: %c\nptr: %c\n", *beg, *ptr);
      // If found next > or <
      if ((*ptr == '<' || *ptr == '>') && !found_arrow)
      {
        found_arrow = true;
        if (!the_word)
          the_word = copy(beg, ptr);
        beg = ptr + 1;
      }
      // If found end of input or output
      else if (found_io && is_operator(*ptr))
      { 
        if (*ptr == '>')
          found_arrow = true;
        else
          found_arrow = false;

        if (*(beg - 1) == '<') 
        {
          found_io = false;
          input = copy(beg, ptr);
          beg = ptr + 1;
        }
        else if (*(beg - 1) == '>') 
        {
          //printf("found output\n");
          found_io = false;
          output = copy(beg, ptr);
          beg = ptr + 1;
        }
      }
      else if (found_arrow && is_operand(*ptr))
          found_io = true;
    }
    // Finish last input or output
    if (found_io)
    {
      if (*(beg - 1) == '<') 
        {
          found_io = false;
          found_arrow = false;
          input = copy(beg, ptr);
        }
        else if (*(beg - 1) == '>') 
        {
          found_io = false;
          found_arrow = false;
          output = copy(beg, ptr);
        }
    } 
    
    if (the_word != NULL)
    {
      *(com->u.word) = delete_white(the_word);
      //printf("word: [%s]\n", the_word);
    }
    if (input != NULL)
    {
      com->input = delete_white(input);
      //printf("input: [%s]\n", input);
    }
    if (output != NULL)
    {
      com->output = delete_white(output);
      //printf("output: [%s]\n", output);
    }
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
  int num_lines = 1;
  char* ptr = beg;
  while (ptr != end)
  {
    if (*ptr == '\n')
      ++num_lines;
    ++ptr;
  }
  return num_lines;
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

  char* string = build_string(get_next_byte, get_next_byte_argument);

  char* end = string + strlen(string) - 1;
    
  //int i = 0;
  char* a = string;
  char* b;

  char* before_comment = a;
  int line_num = 1;
  // Keep track if last non-white character was operator
  bool found_operator = false;
  // Keep track if finished last command or still reading command
  bool finished_command = false;
  // Keep track if found first command in whole script
  bool found_beg_command = false;
  // Keep track if reading comment
  bool found_comment = false;
  b = a;
 
  while (b <= end)
  { 
    //printf("b: <%c>\n", *b);
/*if (found_operator) printf("found_operator = true\n"); else printf("found_operator = false\n");
if (finished_command) printf("finished_command = true\n"); else printf("finished_command = false\n");
if (found_beg_command) printf("found_beg_command = true\n"); else printf("found_beg_command = false\n");
if (found_comment) printf("*******************found_comment = true\n"); else printf("found_comment = false\n");*/
     // Skip over comments
     if (is_operator(*b) && !found_beg_command)
       error(1, 0, "Line %d may not start with operator", line_num);
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
           error(1, 0, "invalid character # at line: %d\n", line_num);
         else found_comment = true;
       }
       found_comment = true;
       before_comment = b - 1;
     }
     //printf("b: <%c>\n", *b);
     else if (*b == '&')
     {
       // Check for beggining with operand
       if (finished_command && !found_comment)
         error(1, 0, "Cannot start with & on line: %d\n", line_num);

       // Check to make sure not last character
       if (b == end && *(b-1) != '&')
         error(1, 0, "Missing second & on line: %d\n", line_num);
       // Check for second &
       else if (*(b+1) != '&' && *(b-1) != '&')
         error(1, 0, "Missing second & on line: %d\n", line_num);
       else if (!found_comment)
         found_operator = true;
     }
     else if (*b == '|')
     {
       // Check for beginnning with operand
       if (finished_command && !found_comment)
         error(1, 0, "Cannot start with | on line: %d\n", line_num);
       else if (!found_comment)
         found_operator = true;
     }
     else if (*b == '\n')
     {
        ++line_num;
        
        // Found end of complete command
        if (!found_operator && found_beg_command && !found_comment)
        {
          //printf("Found end of complete command\n");
          char* temp_string = checked_malloc(sizeof(copy(a, b+1)));
          temp_string = copy(a, b+1);
          //puts(temp_string);
          validate(temp_string, line_num);
          push(stream, make_command(temp_string, 
                                    temp_string + (int)strlen(temp_string), 
                                    line_num));
          a = b + 1;
          finished_command = true;
          found_operator = false;
          found_beg_command = false;
        }
        else if (found_comment) 
        {
          // Found end of comment
          found_comment = false;
          // End of comment, make command preceding it
          if (found_beg_command)
          {
            //printf("Found end of comment and making command!\n");
            char* temp_string = checked_malloc(sizeof(copy(a, before_comment+1)));
            temp_string = copy(a, before_comment+1);
            validate(temp_string, line_num);
            push(stream, make_command(temp_string, 
                                      temp_string + (int)strlen(temp_string), 
                                      line_num));
            a = b + 1;
            finished_command = true;
            found_operator = false;
            found_beg_command = false;
          }
          // Comment came after already made complete command
          else a = b;
        }
     }
     else if (*b != ' ' && *b != '\t' && !found_comment)
     {
       //printf("found_beg_command\n");
       finished_command = false;
       found_operator = false;
       found_beg_command = true;
     }
     ++b;
  }
  
  // Check for unfinished commands
  if (!finished_command)
  {
    char* temp_string;
    //printf("Finishing command:\n");
    temp_string = copy(a, b+1);
    //puts(temp_string);
    validate(temp_string, line_num);
    push(stream, make_command(temp_string,
                              temp_string + (int)strlen(temp_string),
                              line_num));
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


