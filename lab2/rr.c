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
  long response_time;    // amount of time between arrival and first run
  long start_exec_time;  // set when process runs for first time
  long end_exec_time;         // the time at which the process finishes
  long cpu_time;     // cpu time consumed by process
  long wait_time;        // queue wait time of process
  /* End of "Additional fields here" */
};

TAILQ_HEAD (process_list, process);
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


// Here, we compute the median of all the cpu times in oricess queue, rounding to even
long compute_median_runtime(struct process_list *pl) {
    if (TAILQ_EMPTY(pl)) {
        return 1; // If the list is empty, return the median as 1!
    }
    long nprocs = 0;
    struct process *current;
    // Count the number of processes
    TAILQ_FOREACH(current, pl, pointers) {
        nprocs++;
    }
    long* cpu_times = malloc(sizeof(long) * nprocs);
    if (cpu_times == NULL) {
        // Handle malloc failure; depending on the system specifics,
        // you may want to return an error code or exit.
        return -1; // or handle memory allocation error appropriately
    }
    // Collect CPU times
    int i = 0;
    TAILQ_FOREACH(current, pl, pointers) {
        cpu_times[i++] = current->cpu_time;
    }
    // Bubble sort CPU times
    for (int i = 0; i < nprocs - 1; i++) {
        for (int j = 0; j < nprocs - i - 1; j++) {
            if (cpu_times[j] > cpu_times[j + 1]) {
                long temp = cpu_times[j];
                cpu_times[j] = cpu_times[j + 1];
                cpu_times[j + 1] = temp;
            }
        }
    }
    double median = 0.0;
    // Compute the median
    if (nprocs % 2 != 0) {
        median = cpu_times[nprocs / 2];
    } else {
        // For an even number of elements, take the average of the two middle elements
        median = (cpu_times[(nprocs / 2) - 1] + cpu_times[nprocs / 2]) / 2.0;
        
        // If the median is not an integer, round it to the nearest even number
        if (median != (long)median) {
            median = ((long)(median + 0.5)) & ~1;
        }
    }
    free(cpu_times);
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

 // Sort processes by arrival time
sort_process_set(ps);

// Initialize start and cpu times for each process
for (int i = 0; i < ps.nprocesses; i++) {
  ps.process[i].start_exec_time = -1;  // -1 denotes that the process has not started yet
  ps.process[i].cpu_time = 0;     // Initialize with 0 CPU time as none has been used yet
}
long qt = quantum_length;        // Time slice of each process
long next = 0;                   // Index of process that is arriving next
long time = 0;                   // Time counter
struct process *previous = NULL; // Previously executed process
struct process *current = NULL;  // Currently executing process

// Main round-robin scheduling loop
while (true) {
  // Add processes that have arrived before or at the current time
  while (next < ps.nprocesses && time >= ps.process[next].arrival_time) {
    TAILQ_INSERT_TAIL(&list, &ps.process[next], pointers);
    next++;
  }
  // Process switching logic
  if (previous) {
    TAILQ_REMOVE(&list, previous, pointers); // Remove the previous process from the queue
    if (previous->burst_time == previous->cpu_time) {
      // If the process has finished execution
      previous->end_exec_time = time;
      previous->wait_time = previous->end_exec_time - previous->arrival_time - previous->burst_time;
      total_wait_time += previous->wait_time;
    } else {
      // If the process has not finished, re-queue it
      TAILQ_INSERT_TAIL(&list, previous, pointers);
    }
  }
  current = TAILQ_FIRST(&list); // Fetch the process at the front of the queue
  // If no processes are in the queue, jump forward in time to the next process arrival
  if (!current) {
    if (next < ps.nprocesses) {
      time = ps.process[next].arrival_time;
    } else {
      // If there are no more processes to arrive, break out of the loop
      break;
    }
  } else {
    // Context switch overhead if we're switching processes
    if (previous && current != previous) {
      time++; // Increment time to account for context switch
    }
    // If the process is running for the first time, set its start time
    if (current->start_exec_time == -1) {
      current->start_exec_time = time;
      current->response_time = time - current->arrival_time;
      total_response_time += current->response_time;
    }
    // Compute new quantum using median runtime if quantum_length is set to -1
    if (quantum_length == -1) {
      qt = compute_median_runtime(&list);
    }
    // Determine run time for the current process
    long remaining_time = current->burst_time - current->cpu_time;
    long run_time = (qt < remaining_time) ? qt : remaining_time;
    current->cpu_time += run_time; // Update process' CPU time
    time += run_time; // Move forward in time by the run time
  }
  // Set the previous process to the current one for the next iteration
  previous = current;
}

// After the loop, scheduling is complete and all processes have been handled.
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