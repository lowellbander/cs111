// UCLA CS 111 Lab 1 command execution

#include "alloc.h"
#include "command.h"
#include "command-internals.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>

// to keep track of threads running commands in parallel
typedef struct thread_stream *thread_stream_t;
typedef struct thread_node *thread_node_t;

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

void input_dependencies (cmd_stream_t stream)
{
  const int INC = 10;
  size_t size = INC;
  // TODO: Figure out how to grow alloc
  
  stream->curr = stream->tail;
  cmd_node_t current = stream->curr;
  cmd_node_t ptr;
  // Loop through each complete command in stream
  // Starts at tail of stream and goes backwards
  while (current != NULL)
  {
    //printf ("While current != NULL-------------------\n");
    int* ids = checked_malloc(size*sizeof(int));
    int id_size = 0;
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
          //printf("Going through each previous command\n");
          
          // Go through each file in this previous command 
          if (ptr->depends != NULL)
          {
            //printf("Going through each file in previous command\n");
            
            ptr->depends->curr = ptr->depends->head;
            while (ptr->depends->curr != NULL)
            {
              //printf("Comparing files\n");
              // Found dependent file
              //printf("[%s] - - [%s]\n", ptr0->name, ptr->depends->curr->name);
              if (strcmp(ptr0->name, ptr->depends->curr->name) == 0)
              {
                //printf("TWINSIES!\n");
                if (id_size >= (int) size)
                  ids = checked_grow_alloc(ids, &size);
                ids[id_size] = ptr->id;
                ++id_size;
              }
              ptr->depends->curr = ptr->depends->curr->next;
            }
          }
          ptr = ptr->next;
        }
        ptr0 = ptr0->next;
      }
    }
    //printf("break5\n");
    current->depend_id = checked_malloc(sizeof(ids));
    current->depend_id = ids;
    //printf("break6\n");
    // End int array with signal bit (0 can never be an id)
    ids[id_size] = 0;
    //printf("Inputted ID list\n");
    //int x = 0;
    //while (ids[x] != 0)
    //{
    //  printf("%d\n", ids[x]);
    //  ++x;
    //}
    current = current->prev;
  }
  stream->curr = stream->head;
}

// Adds a file_node to the file_stream

void push_file (file_stream_t stream, char* file)
{
  //printf("PUSHING FILE\n");
  file_node_t node = checked_malloc(sizeof(struct file_node));
  node->name = checked_malloc(sizeof(file));
  node->name = file;
  //printf("PUSHED [%s]\n", file);
  node->next = NULL;
  if (stream->head == NULL)
  {
    //printf("Starting stream\n");
    //puts(file);
    // the stream is empty 
    stream->head = node;
    stream->tail = node;
    stream->curr = node;
    return;
  }
  else
  {
    //printf("pushing file\n");
    //puts(file);
    stream->tail->next = node;
    stream->tail = stream->tail->next;
    return;
  }
}

// Retrieves all of the file dependencies and stores them in file stream
// Goes through recursively until looking at the input/output of each
//    simple command

file_stream_t
get_file_depends (command_t c)
{
  //printf ("GETTING FILE DEPENDS\n");
  file_stream_t depends = checked_malloc(sizeof(struct file_stream));
  depends->head = NULL;
  depends->curr = NULL;
  depends->tail = NULL;
  // Complete command is just a simple command
  switch (c->type)
  {
    case SIMPLE_COMMAND:
    {
      //printf("depending on simple\n");
      //printf("word: %s\n", *c->u.word);
      if (c->input != NULL)
      {
        //printf("input: [");
        //puts(c->input);
        //printf("]\n");
        push_file(depends, c->input);
      }
      if (c->output != NULL)
        push_file(depends, c->output);
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
      //printf("depending on pipe\n");
      // Set depends to left dependency list
      depends = get_file_depends (c->u.command[0]);
      //printf("Got depends\n");
      // Append right depedency list to tail of current list
      if (depends->curr != NULL)
      {
        //printf("Found left dependency....\n");
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
      {
        //printf("Found no right dependency\n");
        depends = get_file_depends(c->u.command[1]);
      }
    }
  }
  //printf("out of switch\n");
  depends->curr = depends->head;
  return depends;
}

//////////////////////////////////////////////////////////
///////// Prints dependency file list (delete after debug)
void print_file_depends (file_stream_t files)
{
  files->curr = files->head;
  printf("Depends on:\n");
  while (files->curr != NULL)
  {
    printf("[%s]\n", files->curr->name);
    files->curr = files->curr->next;
  }
  printf("\n");
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
    //printf("ID: %d------------------\n", id);
    cmd_node_t node = checked_malloc(sizeof(struct cmd_node));
    node->id = id;
    node->self = command;
    node->next = NULL;
    node->depends = checked_malloc(sizeof(struct file_stream));
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
execute_command (command_t c, int time_travel)
{
  if (time_travel == 0)
    system(build_sys_string(c));
}

void* hello(void* void_ptr)
{
  printf("HELLO, WORLD!\n");
  return NULL;
}

struct thread_node
{
  pthread_t thread;

};

struct thread_stream
{
  thread_node_t head;
  thread_node_t curr;
  // maybe make it circular so we don't need a head?
  // but then how to check that every thread has run?
};

void
run(command_t command)
{
  //get_or_create_thread()
  
}

command_t
get_command(command_stream_t command)
{
  //cmd_node_t = 
  return NULL;
}

void
exe_stream (command_stream_t stream, int time_travel)
{
  if (time_travel == 0)
  {
    command_t command;
    while ((command = read_command_stream(stream)))
      execute_command(command, time_travel);
  }
  else
  {
    //group together the commands by dependencies, possible into command streams
    //then execute each command stream in it's own thread

    cmd_stream_t cmds = checked_malloc(sizeof(struct cmd_stream));
    cmds = initialize_cmds(stream);
    
    
    // Print dependencies for debugging
    cmds->curr = cmds->head;
    int id = 1;
    printf("PRINTING FILE DEPENDENCIES\n");
    while (cmds->curr != NULL)
    {
      printf("ID: %d\n", id);
      file_stream_t file_stream = cmds->curr->depends;
      print_file_depends(file_stream);
      cmds->curr = cmds->curr->next;
      ++id;
    }
    
    input_dependencies (cmds);
    printf("PRINTING COMMAND DEPENDENCIES\n");
    id = 1;
    while (cmds->curr != NULL)
    {
      int i = 0;
      printf("ID: %d\n", id);
      printf("Depends on:\n");
      while (cmds->curr->depend_id[i] != 0)
      {
        printf("[%d]\n", cmds->curr->depend_id[i]);
        ++i;
      }
      ++id;
      cmds->curr = cmds->curr->next;
    }
    
    thread_stream_t thread_stream = checked_malloc(sizeof(thread_stream));
    cmds->curr = cmds->head;


    //pthread_t* ptr = malloc(sizeof(pthread_t));
  
    //pthread_create(ptr, NULL, &hello, NULL);
    //pthread_join(*ptr, NULL);
    
    //pthread_t t1;
    //pthread_create(&t1, NULL, &hello, NULL);
    //pthread_join(t1, NULL);
    
    //error(1, 0, "time travel not yet implemented\n");
  }
}
