// UCLA CS 111 Lab 1 command execution

#include "alloc.h"
#include "command.h"
#include "command-internals.h"
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <error.h>


// to keep track of threads running commands in parallel
typedef struct thread_stream *thread_stream_t;
typedef struct thread_node *thread_node_t;

//global list of threads
thread_stream_t threads;

typedef struct file_stream *file_stream_t;
typedef struct file_node *file_node_t;

// Used for execution, includes dependencies
typedef struct cmd_stream *cmd_stream_t;
typedef struct cmd_node *cmd_node_t;

// Stream of file name dependencies
struct file_stream
{ 
  file_node_t head;
  file_node_t tail;
  file_node_t curr;
};

struct file_node
{
  char* name;
  // If command needs to read or write to this file (r or w)
  char type;
  file_node_t next;
};		

struct cmd_stream
{
  cmd_node_t head;
  cmd_node_t curr;
  cmd_node_t tail;
};

struct cmd_node
{
  // id starts at 1
  int id;
  command_t self;
  cmd_node_t next;
  cmd_node_t prev;
  file_stream_t depends;
  // id of cmd_nodes it is dependent on, end of array = 0
  // After cmd_node id finishes, id turns negative
  int* depend_id;
};		

struct thread_node
{
  pthread_t* thread;
  thread_node_t next;
  cmd_node_t command_node;
};

struct thread_stream
{
  thread_node_t head;
  thread_node_t tail;
};

void input_dependencies (cmd_stream_t stream)
{
  const int INC = sizeof(int);
  size_t size = INC;
  size_t id_size = size;
  
  stream->curr = stream->tail;
  cmd_node_t current = stream->curr;
  cmd_node_t ptr;
  // Loop through each complete command in stream
  // Starts at tail of stream and goes backwards
  
  while (current != NULL)
  {
    size = INC;
    int* ids = checked_malloc(size*sizeof(int));
    int id_index = 0;
    ids[0] = 0; 
    ptr = stream->head;

    // Check each file dependency 
    if (current->depends != NULL)
    {
      current->depends->curr = current->depends->head;
      file_node_t ptr0 = current->depends->curr;
      
      // Go through each file in current command
      while (ptr0 != NULL)
      {
        //printf("Going through each file in current command\n");
        ptr = stream->head;
        // Go through each previous command
        while (ptr != current)
        {
          // Go through each file in this previous command 
          if (ptr->depends != NULL)
          {
            ptr->depends->curr = ptr->depends->head;
            while (ptr->depends->curr != NULL)
            {
              // Found dependent file
              if (strcmp(ptr0->name, ptr->depends->curr->name) == 0 &&
                  (ptr->depends->curr->type == 'w' || ptr0->type == 'w'))
              {
                int j = 0;
                while (j < id_index)
                {
                  if (ids[j] == ptr->id)
                    break;
                  ++j;
                }
                // If command id was not already in list, add it
                if (j == id_index)
                {
                  ids = checked_grow_alloc(ids, &size);
                  ids[id_index] = ptr->id;
                  ++id_index;
                  int y;
                }
              }
              ptr->depends->curr = ptr->depends->curr->next;
            }
          }
          ptr = ptr->next;
        }
        ptr0 = ptr0->next;
      }
    }
    current->depend_id = ids;
    // End int array with signal bit (0 can never be an id)
    ids[id_index] = 0;

    current = current->prev;
  }

  stream->curr = stream->head;
}

// Adds a file_node to the file_stream

void push_file (file_stream_t stream, char* file, char type)
{
  file_node_t node = checked_malloc(sizeof(struct file_node));
  node->name = file;
  node->type = type;
  node->next = NULL;
  if (stream->head == NULL)
  {
    // the stream is empty 
    stream->head = node;
    stream->tail = node;
    stream->curr = node;
    return;
  }
  else
  {
    stream->tail->next = node;
    stream->tail = stream->tail->next;
    return;
  }
}

// Retrieves all of the file dependencies and stores them in file stream
// Goes through recursively until looking at the input/output of each
//    simple command
// char prev_type represents the Read/write type of outer command and
//    is used to keep track if coming from pipe

file_stream_t
get_file_depends (command_t c)
{
  file_stream_t depends = checked_malloc(sizeof(struct file_stream));
  depends->head = NULL;
  depends->curr = NULL;
  depends->tail = NULL;
  // Complete command is just a simple command
  switch (c->type)
  {
    case SIMPLE_COMMAND:
    {
      if (c->input != NULL)
        push_file(depends, c->input, 'r');

      if (c->output != NULL)
        push_file(depends, c->output, 'w');
      
      char* word = *c->u.word;
      if (word != NULL)
      {
        int word_size = strlen(word);
        int i = 0;
        int beg = 0;
        // Tokenize word
        while (i < word_size)
        {
          char temp_ch = word[i];
          // Found end of token
          if (temp_ch == ' ' || temp_ch == '\n')
          {
            char* token = checked_malloc((i-beg)*sizeof(char));
            int t = beg;
            int index = 0;
            while (t < i)
            {
              token[index] = word[t];
              ++index;
              ++t;
            }
            push_file (depends, token, 'r');
            beg = i + 1;
          } 
          ++i;
        }
        
        int index = 0;
        // Finish last token
        if (beg < i)
        {
          char* token = checked_malloc((i-beg)*sizeof(char));
          while (beg < i)
          {
            token[index] = word[beg];
            ++index;
            ++beg;
          }
          push_file (depends, token, 'r');
        }
      }
      break;
    }
    case SUBSHELL_COMMAND:
    {
      depends = get_file_depends (c->u.subshell_command);
      break;
    }
    case AND_COMMAND:
    case OR_COMMAND:
    case SEQUENCE_COMMAND:
    case PIPE_COMMAND:
    {
      // Set depends to left dependency list
      depends = get_file_depends (c->u.command[0]);
      // Append right depedency list to tail of current list
      if (depends->curr != NULL)
      {
        depends->tail->next = (get_file_depends(c->u.command[1]))->head;
        // Update tail
        depends->curr = depends->tail;
        while(depends->curr->next != NULL)
        {
          depends->curr = depends->curr->next;
        }
        depends->tail = depends->curr;
        break;
      }
      else
        depends = get_file_depends(c->u.command[1]);
    }
  }
  depends->curr = depends->head;
  return depends;
}

// Set up's cmd_stream, minus depend_id
cmd_stream_t
initialize_cmds (command_stream_t command_stream)
{
  cmd_stream_t stream = checked_malloc(sizeof(struct cmd_stream));
  command_t command;
  
  int id = 1;
  while ((command = read_command_stream(command_stream)))
  {
    cmd_node_t node = checked_malloc(sizeof(struct cmd_node));
    node->id = id;
    node->self = command;
    node->next = NULL;
    node->depends = get_file_depends(command);
    
    if (stream->head == NULL)
    {
      node->prev = NULL;
      stream->head = node;
      stream->curr = node;
    }
    else
    {
      node->prev = stream->curr;
      stream->curr->next = node;
      stream->curr = stream->curr->next;
    }
    ++id;
  }
  stream->tail = stream->curr;
  stream->curr = stream->head;
  return stream;
}

int
command_status (command_t c)
{
  return c->status;
}

// Finds length of command string 

int
find_command_size (command_t c)
{
  int len = 0;
  switch (c->type)
  {
    case SIMPLE_COMMAND:
    {
      len = sizeof(*(c->u.word)) + sizeof(*(c->input)) 
            + sizeof(*(c->output)) + 2;
      break;
    }
    case SUBSHELL_COMMAND:
    {
      len = 2 + find_command_size(c->u.command[0]);
      break;
    }
    case PIPE_COMMAND:
    case SEQUENCE_COMMAND:
    {
      len = find_command_size(c->u.command[0]) 
            + find_command_size(c->u.command[1]) + 1;
      break;
    }
    case AND_COMMAND:
    case OR_COMMAND:
    {
      len = find_command_size(c->u.command[0]) 
            + find_command_size(c->u.command[1]) + 2;
      break;
    }
  }
  return len;
}

// Builds command string to pass into system()
// It is called recursively, appending simple commands and the operators

char*
build_sys_string (command_t c)
{
  int len = find_command_size(c);
  char* word = checked_malloc(len);
  
  switch (c->type)
  {
    case SIMPLE_COMMAND:
    {
      strcat(word, *(c->u.word));
      if (c->input != NULL)
      {
        strcat(word, "<");
        strcat(word, c->input);
      }
      if (c->output != NULL)
      {
        strcat(word, ">");
        strcat(word, c->output);
      }
      break;
    }
    case SUBSHELL_COMMAND:
    {
      strcat(word, "(");
      strcat(word, build_sys_string(c->u.subshell_command));
      strcat(word, ")");
      break;
    }
    case SEQUENCE_COMMAND:
    {
      strcat(word, build_sys_string(c->u.command[0]));
      strcat(word, ";");
      strcat(word, build_sys_string(c->u.command[1]));
      break;
    }
    case AND_COMMAND:
    {
      strcat(word, build_sys_string(c->u.command[0]));
      strcat(word, "&&");
      strcat(word, build_sys_string(c->u.command[1]));
      break;
    }
    case OR_COMMAND:
    {
      strcat(word, build_sys_string(c->u.command[0]));
      strcat(word, "||");
      strcat(word, build_sys_string(c->u.command[1]));
      break;
    }
    case PIPE_COMMAND:
    {
      strcat(word, build_sys_string(c->u.command[0]));
      strcat(word, "|");
      strcat(word, build_sys_string(c->u.command[1]));
      break;
    }
  }
  return word;
}

void
execute_command (command_t c)
{
  c->status = system(build_sys_string(c));
}

thread_node_t 
get_thread_node(int id)
{
  thread_node_t node = threads->head;
  while (node->command_node->id != id)
    node = node->next;

  //spin until command completes
  while (node->command_node->self->status < 0)
    continue;
    
  return node;
}

void* run(void* context)
{
  cmd_node_t node = context;

  // illustrates that commands not run sequentially
  //sleep(rand() % 5); 

  // wait for all dependencies to finish execution
  int i;
  int len = sizeof(node->depend_id)/sizeof(int);
  for (i = 0; node->depend_id[i] != 0; ++i)
  {
    thread_node_t t = get_thread_node(node->depend_id[i]);
    pthread_join(*t->thread, NULL);
  }
  execute_command(node->self);
  return NULL;
}

void
exe_stream (command_stream_t stream, int time_travel)
{
  if (time_travel == 0)
  {
    command_t command;
    while ((command = read_command_stream(stream)))
      execute_command(command);
  }
  else
  {
    
    cmd_stream_t cmds = initialize_cmds(stream);
    input_dependencies (cmds);
    
    //build a list of threads, one per command
    threads = checked_malloc(sizeof(struct thread_stream));
    cmd_node_t ptr = cmds->head;
    threads->head = NULL;
    while (ptr)
    {
      thread_node_t t = checked_malloc(sizeof(struct thread_node));
      t->next = NULL;
      t->command_node = ptr;
      t->thread = checked_malloc(sizeof(pthread_t));

      if (threads->head == NULL)
        threads->head = t;
      else
        threads->tail->next = t;

      threads->tail = t;
      ptr = ptr->next;
    }

    // execute each command in its own thread
    thread_node_t t = threads->head;
    while (t)
    {
      pthread_create(t->thread, NULL, &run, t->command_node);
      t = t->next;
    }

    //wait for all the threads to finish
    t = threads->head;
    while (t)
    {
      pthread_join(*t->thread, NULL);
      t = t->next;
    }
  }
}
