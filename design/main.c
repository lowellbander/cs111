// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "command.h"

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-pt] SCRIPT-FILE [N-THREADS]", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  int opt;
  int command_number = 1;
  int print_tree = 0;
  int time_travel = 0;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "pt"))
      {
      case 'p': print_tree = 1; break;
      case 't': time_travel = 1; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // check for valid syntax
  if (argc != 2 && argc != 3 && argc != 4)
    usage ();

  char* pEnd;
  //int nThreads = (argc == 4) ? strtol(argv[argc-1], &pEnd, 10) : -1;
  int nThreads = -1;
  if (argc == 4)
  {
    int n = strtol(argv[argc-1], &pEnd, 10);
    if (n <= 0)
      error(1, 0, "N-THREADS must be positive\n");
    else
      nThreads = n;
  }

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);

  command_t last_command = NULL;
  command_t command;
  if (print_tree)
    while ((command = read_command_stream (command_stream)))
	  {
	    printf ("# %d\n", command_number++);
	    print_command (command);
	  }
  else
    exe_stream(command_stream, time_travel, nThreads);

  return print_tree || !last_command ? 0 : command_status (last_command);
}
