#include <fcntl.h>
#include <stdbool.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <unistd.h>

/* A process table entry.  */
struct process
{
  long pid;
  long arrival_time;
  long burst_time;

  TAILQ_ENTRY (process) pointers;

  /* Additional fields here */
  long start_time;  // the time at which the process first runs
  long end_time;         // the time at which the process finishes
  long cpu_time;     // amount of time that process has consumed the cpu
  long response_time;    // amount of time between arrival and first run
  long wait_time;        // amount of time that process was just waiting in queue
  /* End of "Additional fields here" */
};

TAILQ_HEAD (process_list, process);

/* Skip past initial nondigits in *DATA, then scan an unsigned decimal
   integer and return its value.  Do not scan past DATA_END.  Return
   the integer’s value.  Report an error and exit if no integer is
   found, or if the integer overflows.  */
static long
next_int (char const **data, char const *data_end)
{
  long current = 0;
  bool int_start = false;
  char const *d;

  for (d = *data; d < data_end; d++)
    {
      char c = *d;
      if ('0' <= c && c <= '9')
	{
	  int_start = true;
	  if (ckd_mul (&current, current, 10)
	      || ckd_add (&current, current, c - '0'))
	    {
	      fprintf (stderr, "integer overflow\n");
	      exit (1);
	    }
	}
      else if (int_start)
	break;
    }

  if (!int_start)
    {
      fprintf (stderr, "missing integer\n");
      exit (1);
    }

  *data = d;
  return current;
}

/* Return the first unsigned decimal integer scanned from DATA.
   Report an error and exit if no integer is found, or if it overflows.  */
static long
next_int_from_c_str (char const *data)
{
  return next_int (&data, strchr (data, 0));
}

/* A vector of processes of length NPROCESSES; the vector consists of
   PROCESS[0], ..., PROCESS[NPROCESSES - 1].  */
struct process_set
{
  long nprocesses;
  struct process *process;
};

/* Return a vector of processes scanned from the file named FILENAME.
   Report an error and exit on failure.  */
static struct process_set
init_processes (char const *filename)
{
  int fd = open (filename, O_RDONLY);
  if (fd < 0)
    {
      perror ("open");
      exit (1);
    }

  struct stat st;
  if (fstat (fd, &st) < 0)
    {
      perror ("stat");
      exit (1);
    }

  size_t size;
  if (ckd_add (&size, st.st_size, 0))
    {
      fprintf (stderr, "%s: file size out of range\n", filename);
      exit (1);
    }

  char *data_start = mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
    {
      perror ("mmap");
      exit (1);
    }

  char const *data_end = data_start + size;
  char const *data = data_start;

  long nprocesses = next_int (&data, data_end);
  if (nprocesses <= 0)
    {
      fprintf (stderr, "no processes\n");
      exit (1);
    }

  struct process *process = calloc (sizeof *process, nprocesses);
  if (!process)
    {
      perror ("calloc");
      exit (1);
    }

  for (long i = 0; i < nprocesses; i++)
    {
      process[i].pid = next_int (&data, data_end);
      process[i].arrival_time = next_int (&data, data_end);
      process[i].burst_time = next_int (&data, data_end);
      if (process[i].burst_time == 0)
	{
	  fprintf (stderr, "process %ld has zero burst time\n",
		   process[i].pid);
	  exit (1);
	}
    }

  if (munmap (data_start, size) < 0)
    {
      perror ("munmap");
      exit (1);
    }
  if (close (fd) < 0)
    {
      perror ("close");
      exit (1);
    }
  return (struct process_set) {nprocesses, process};
}

// Sorting the process set by the arrival time
void sort_process_set(struct process_set ps) {
  // Flag to check if swapping has occurred, to potentially reduce the number of passes
  int swapped;
  
  for (long i = 0; i < ps.nprocesses - 1; i++) {
    // Initialize swapped to zero (false) at the beginning of each pass
    swapped = 0;
    
    // Perform a pass of bubble sort
    for (long j = 0; j < ps.nprocesses - i - 1; j++) {
      // Compare adjacent processes by their arrival time
      if (ps.process[j].arrival_time > ps.process[j + 1].arrival_time) {
        // Swap the processes if they are out of order
        struct process temp = ps.process[j];
        ps.process[j] = ps.process[j + 1];
        ps.process[j + 1] = temp;

        // Set swapped to one (true) to indicate that a swap occurred
        swapped = 1;
      }
    }

    // If no two elements were swapped by inner loop, then the array is sorted
    if (swapped == 0) {
      break;
    }
  }
}


// Compute the median of all cpu times in a queue of processes, rounding to even
long compute_median_runtime(struct process_list *pl) {
  double median = 0.0;
  if (!TAILQ_EMPTY(pl)) {
    struct process *current;

    // 1. get number of procs in queue
    long nprocs = 0;
    TAILQ_FOREACH(current, pl, pointers) {
      nprocs++;
    }

    // 2. collect all cpu times into an array
    long* cpu_times = malloc(sizeof(long) * nprocs);
    int i = 0;
    TAILQ_FOREACH(current, pl, pointers) {
      cpu_times[i] = current->cpu_time;
      i++;
    }

    // 3. sort the array
    for (int i = 0; i < (nprocs-1); i++) {
      for (int j = 0; j < (nprocs-i-1); j++) {
        if (cpu_times[j] > cpu_times[j+1]) {
          long temp = cpu_times[j];
          cpu_times[j] = cpu_times[j+1];
          cpu_times[j+1] = temp;
        }
      }
    }

    // 4. compute them median
    if (nprocs % 2 != 0) {
      median = cpu_times[nprocs/2];
    }
    else {
      median = (cpu_times[(nprocs/2)-1] + cpu_times[nprocs/2]) / 2;
      // round to even if non-whole number
      int median_int = (int)median;
      if (median - median_int != 0) {
        median = (median_int % 2 == 0) ? (double)median_int : (double)(median_int + 1);
      }
    }

    free(cpu_times);
  }

  // clamp median to 1
  if (median == 0)
    median = 1;

  return (long)median;
}

int
main (int argc, char *argv[])
{
  if (argc != 3)
    {
      fprintf (stderr, "%s: usage: %s file quantum\n", argv[0], argv[0]);
      return 1;
    }

  struct process_set ps = init_processes (argv[1]);
  long quantum_length = (strcmp (argv[2], "median") == 0 ? -1
			 : next_int_from_c_str (argv[2]));
  if (quantum_length == 0)
    {
      fprintf (stderr, "%s: zero quantum length\n", argv[0]);
      return 1;
    }

  struct process_list list;
  TAILQ_INIT (&list);

  long total_wait_time = 0;
  long total_response_time = 0;

  /* Your code here */

  // sort processes by arrival time
  sort_process_set(ps);
  
  // initialize start and cpu times for each process
  for (int i = 0; i < ps.nprocesses; i++) {
    ps.process[i].start_time = -1;
    ps.process[i].cpu_time = 0;
  }

  struct process *prev_process = NULL;  // the process that previously ran
  struct process *curr_process = NULL;  // the process that previously ran
  long quantum = quantum_length;  // the quantum length that the process is assigned
  long next_arrival = 0;  // the index of the process that is arriving next
  long time = 0;  // overall time elapsed (skip to first arrival time)
  for(;;) {
    // add any processes that arrived before the current time
    for (;;next_arrival++) {
      if (next_arrival < ps.nprocesses && time > ps.process[next_arrival].arrival_time)
        TAILQ_INSERT_TAIL(&list, &ps.process[next_arrival], pointers);
      else
        break;
    }

    // if a process just finished or reached the end of its quantum
    if (prev_process != NULL) {
      TAILQ_REMOVE(&list, prev_process, pointers);
      // if process finished
      if (prev_process->burst_time == prev_process->cpu_time) {
        prev_process->end_time = time;
        prev_process->wait_time = prev_process->end_time - prev_process->arrival_time - prev_process->burst_time;
        total_wait_time += prev_process->wait_time;
      }
      else
        // re-insert process at tail of queue
        TAILQ_INSERT_TAIL(&list, prev_process, pointers);
    }

    // add any processes that arrive at the current time
    for (;;next_arrival++) {
      if (next_arrival < ps.nprocesses && time == ps.process[next_arrival].arrival_time)
        TAILQ_INSERT_TAIL(&list, &ps.process[next_arrival], pointers);
      else
        break;
    }

    curr_process = TAILQ_FIRST(&list);

      struct process* foo;
      TAILQ_FOREACH(foo, &list, pointers) {
      }
      

    // seek to next arrival time if no procs in queue
    if (curr_process == NULL) {
      time = ps.process[next_arrival].arrival_time;
    }
    // run process at front of queue
    else {
      // context switch if necessary
      if (prev_process != NULL && curr_process != prev_process) {
        time++;
      }

      // if it's the processes first time running
      if (curr_process->start_time == -1) {
        curr_process->start_time = time;
        curr_process->response_time = curr_process->start_time - curr_process->arrival_time;
        total_response_time += curr_process->response_time;
      }

      // compute new quantum if using median
      if (quantum_length == -1) {
        quantum = compute_median_runtime(&list);
      }

      // run process for min(quantum, remaining time)
      long remaining_time = curr_process->burst_time - curr_process->cpu_time;
      long run_time = (quantum < remaining_time) ? quantum : remaining_time;
      curr_process->cpu_time += run_time;
      time += run_time;
    }

    prev_process = curr_process;

    // exit if no remaining processes
    if (curr_process == NULL && next_arrival == ps.nprocesses)
      break;
  }

  /* End of "Your code here" */

  printf ("Average wait time: %.2f\n",
	  total_wait_time / (double) ps.nprocesses);
  printf ("Average response time: %.2f\n",
	  total_response_time / (double) ps.nprocesses);

  if (fflush (stdout) < 0 || ferror (stdout))
    {
      perror ("stdout");
      return 1;
    }

  free (ps.process);
  return 0;
}