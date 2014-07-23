// UCLA CS 111 Lab 1 command execution

#include "alloc.h"
#include "command.h"
#include "command-internals.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
   
typedef struct file_stream *file_stream_t;
typedef struct file_node *file_node_t;

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

// Adds a file_node to the file_stream

void push_file (file_stream_t stream, char* file)
{
  file_node_t node = checked_malloc(sizeof(struct file_node));
  node->name = checked_malloc(sizeof(file));
  *(node->name) = *file;
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
  file_stream_t depends = checked_malloc(sizeof(struct file_stream));
  // Complete command is just a simple command
  switch (c->type)
  {
    case SIMPLE_COMMAND:
    {
      //printf("depending on simple\n");
      if (c->input != NULL)
        push_file(depends, c->input);
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
      // Append right depedency list to tail of current list
      depends->tail->next = (get_file_depends (c->u.command[1]))->head;
      
      // Update tail
      depends->curr = depends->tail->next;
      while(depends->curr->next != NULL)
      {
        depends->curr = depends->curr->next;
      }
      depends->tail = depends->curr;
      break;
    }
  }
    
  return depends;
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

//void* hello(void* void_ptr)
//{
//  printf("hello, world!\n");
//  return NULL;
//}

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

    //pthread_t t1;
    //pthread_create(&t1, NULL, &hello, NULL);
    //pthread_join(t1, NULL);
    
    file_stream_t files = checked_malloc(sizeof(struct file_stream));
    files = get_file_depends(read_command_stream(stream));
    files->curr = files->head;
    
    printf("Depends on:\n");
    while (files->curr != NULL)
    {
      printf("[%s]\n", files->curr->name);
      files->curr = files->curr->next;
    }
    error(1, 0, "time travel not yet implemented\n");
  }
}
