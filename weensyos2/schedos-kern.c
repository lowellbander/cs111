#include "schedos-kern.h"
#include "x86.h"
#include "lib.h"

/*****************************************************************************
 * schedos-kern
 *
 *   This is the schedos's kernel.
 *   It sets up process descriptors for the 4 applications, then runs
 *   them in some schedule.
 *
 *****************************************************************************/

// The program loader loads 4 processes, starting at PROC1_START, allocating
// 1 MB to each process.
// Each process's stack grows down from the top of its memory space.
// (But note that SchedOS processes, like MiniprocOS processes, are not fully
// isolated: any process could modify any part of memory.)

#define NPROCS		5
#define PROC1_START	0x200000
#define PROC_SIZE	0x100000

// +---------+-----------------------+--------+---------------------+---------/
// | Base    | Kernel         Kernel | Shared | App 0         App 0 | App 1
// | Memory  | Code + Data     Stack | Data   | Code + Data   Stack | Code ...
// +---------+-----------------------+--------+---------------------+---------/
// 0x0    0x100000               0x198000 0x200000              0x300000
//
// The program loader puts each application's starting instruction pointer
// at the very top of its stack.
//
// System-wide global variables shared among the kernel and the four
// applications are stored in memory from 0x198000 to 0x200000.  Currently
// there is just one variable there, 'cursorpos', which occupies the four
// bytes of memory 0x198000-0x198003.  You can add more variables by defining
// their addresses in schedos-symbols.ld; make sure they do not overlap!


// A process descriptor for each process.
// Note that proc_array[0] is never used.
// The first application process descriptor is proc_array[1].
static process_t proc_array[NPROCS];

// A pointer to the currently running process.
// This is kept up to date by the run() function, in mpos-x86.c.
process_t *current;

// The preferred scheduling algorithm.
int scheduling_algorithm;


/*****************************************************************************
 * start
 *
 *   Initialize the hardware and process descriptors, then run
 *   the first process.
 *
 *****************************************************************************/

void
start(void)
{
	int i;

	// Set up hardware (schedos-x86.c)
	segments_init();
	interrupt_controller_init(1);
	console_clear();

	// Initialize process descriptors as empty
	memset(proc_array, 0, sizeof(proc_array));
	for (i = 0; i < NPROCS; i++) {
		proc_array[i].p_pid = i;
		proc_array[i].p_state = P_EMPTY;
	}

	// Set up process descriptors (the proc_array[])
	for (i = 1; i < NPROCS; i++) {
		process_t *proc = &proc_array[i];
		uint32_t stack_ptr = PROC1_START + i * PROC_SIZE;

		// Initialize the process descriptor
		special_registers_init(proc);

		// Set ESP
		proc->p_registers.reg_esp = stack_ptr;

		// Load process and set EIP, based on ELF image
		program_loader(i - 1, &proc->p_registers.reg_eip);

		// Mark the process as runnable!
		proc->p_state = P_RUNNABLE;
		
		// Initialize all priority levels to 0 (highest priority)
		// This lets all processes run once so they may set their own priority
		proc->p_priority = 0;
		proc->p_times_run = 0;
	}

	// Initialize the cursor-position shared variable to point to the
	// console's first character (the upper left).
	cursorpos = (uint16_t *) 0xB8000;

	// Initialize the scheduling algorithm.
	scheduling_algorithm = 3;

	// Switch to the first process
	run(&proc_array[1]);

	// Should never get here!
	while (1)
		/* do nothing */;
}



/*****************************************************************************
 * interrupt
 *
 *   This is the weensy interrupt and system call handler.
 *   The current handler handles 4 different system calls (two of which
 *   do nothing), plus the clock interrupt.
 *
 *   Note that we will never receive clock interrupts while in the kernel.
 *
 *****************************************************************************/

void
interrupt(registers_t *reg)
{
	// Save the current process's register state
	// into its process descriptor
	current->p_registers = *reg;

	switch (reg->reg_intno) {

	case INT_SYS_YIELD:
		// The 'sys_yield' system call asks the kernel to schedule
		// the next process.
		schedule();

	case INT_SYS_EXIT:
		// 'sys_exit' exits the current process: it is marked as
		// non-runnable.
		// The application stored its exit status in the %eax register
		// before calling the system call.  The %eax register has
		// changed by now, but we can read the application's value
		// out of the 'reg' argument.
		// (This shows you how to transfer arguments to system calls!)
		current->p_state = P_ZOMBIE;
		current->p_exit_status = reg->reg_eax;
		schedule();

	case INT_SYS_SET_PRIORITY:
		// 'sys_set_priority' sets the priority of the process
		// Process is allowed to set its own priority
		current->p_priority = reg->reg_eax;
		// Let all processes set priority first before running anything
		if (current->p_pid == (NPROCS - 1))
			schedule();
		else
			run(current+1);

	case INT_SYS_PRINT:
		// Prints character to console (will not be interrupted)
		*cursorpos++ = reg->reg_eax;
		schedule();

	case INT_CLOCK:
		// A clock interrupt occurred (so an application exhausted its
		// time quantum).
		// Switch to the next runnable process.
		schedule();

	default:
		while (1)
			/* do nothing */;

	}
}

// a pseudo-random number generator 
// implemented using a Linear Feedback Shift Register
unsigned short lfsr = 0xACE1u;
unsigned bit;

unsigned rand()
{
  bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
  return lfsr =  (lfsr >> 1) | (bit << 15);
}


/*****************************************************************************
 * schedule
 *
 *   This is the weensy process scheduler.
 *   It picks a runnable process, then context-switches to that process.
 *   If there are no runnable processes, it spins forever.
 *
 *   This function implements multiple scheduling algorithms, depending on
 *   the value of 'scheduling_algorithm'.  We've provided one; in the problem
 *   set you will provide at least one more.
 *
 *****************************************************************************/

#define PROB1 25
#define PROB2 25
#define PROB3 25
#define PROB4 25

void
schedule(void)
{
	pid_t pid = current->p_pid;

  // Round Robin
	if (scheduling_algorithm == 0)
		while (1) {
			pid = (pid + 1) % NPROCS;

			// Run the selected process, but skip
			// non-runnable processes.
			// Note that the 'run' function does not return.
			if (proc_array[pid].p_state == P_RUNNABLE)
				run(&proc_array[pid]);
		}
	
	// Exercise 2:
	// Priority Scheduling	
	else if (scheduling_algorithm == 1)
	{
	  // Start at the highest priority
	  pid = 1;
	  while (1)
	  {
	    if (proc_array[pid].p_state == P_RUNNABLE)
	      run(&proc_array[pid]);
	    else
	      pid = (pid + 1) % NPROCS;
	  }
	}
	
	// Exercise 4A:
	// Priority scheduling, but processes may set their own priority level
	else if (scheduling_algorithm == 2)
	{
		int temp_pid = -1;
		int i = 1;
		// Go through all processes, loop back if never found a RUNNABLE process
		while ((i < NPROCS && NPROCS > 1) || temp_pid == -1)
		{
				if (proc_array[i].p_state == P_RUNNABLE)
				{
					// First RUNNABLE process found or
					// Found process with higher priority or
					// Found process with same priority but has run less times
					if (temp_pid == -1 ||
							proc_array[i].p_priority < proc_array[temp_pid].p_priority ||
							(proc_array[temp_pid].p_priority == proc_array[i].p_priority &&
					      proc_array[i].p_times_run <= proc_array[temp_pid].p_times_run))
						temp_pid = i;
				}
				// Never found a RUNNABLE process after going through all of them
				// Loop back to find one
				if (temp_pid == -1 && i == NPROCS - 1)
					i = 1;
				else
					++i;
  	}
  	// Increment times_run for this process and run
 		proc_array[temp_pid].p_times_run++;
 		run(&proc_array[temp_pid]);
	}


  // Exercise 7:
  // Lottery scheduling
  else if (scheduling_algorithm == 3)
  {
    int tickets[100];
    int i;
    int sum = PROB1 + PROB2 + PROB3 + PROB4;
    if (sum != 100)
    {
	    cursorpos = console_printf(cursorpos, 0x100, 
            "\n Probabilities must sum to 100, not %d\n", sum);
      return;
    }
    //initialize tickets
    for (i = 0; i < PROB1; ++i)
      tickets[i] = 1;
    for (     ; i < PROB1 + PROB2; ++i)
      tickets[i] = 2;
    for (     ; i < PROB1 + PROB2 + PROB3; ++i)
      tickets[i] = 3;
    for (     ; i < sum; ++i)
      tickets[i] = 4;

    int index;
    while (1)
    {
      index = rand() % sum; // 0 < index < 99
      pid = tickets[index];
			if (proc_array[pid].p_state == P_RUNNABLE)
				run(&proc_array[pid]);
    }
  }

	// If we get here, we are running an unknown scheduling algorithm.
	cursorpos = console_printf(cursorpos, 0x100, 
              "\nUnknown scheduling algorithm %d\n", scheduling_algorithm);
	while (1)
		/* do nothing */;
}
