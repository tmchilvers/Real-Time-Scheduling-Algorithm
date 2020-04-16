/*
* Programmed by Tristan Chilvers
* ID: 2288893
* Email: chilvers@chapman.edu
*
*	References:
*     HELPER METHODS:
*			- https://www.geeksforgeeks.org/lcm-of-given-array-elements/
*			- https://www.geeksforgeeks.org/recursive-bubble-sort/
*
*     GENERAL MULTITHREADING:
*     - https://www.geeksforgeeks.org/multithreading-c-2/
*     - https://www.tutorialspoint.com/multithreading-in-c
*     - http://www.csc.villanova.edu/~mdamian/threads/posixsem.html
*
*     GENERAL POINTER DEBUGGING:
*     - https://www.codeproject.com/Questions/634865/ERROR-dereferencing-pointer-to-incomplete-type
*     - http://www.cplusplus.com/reference/cstdio/sscanf/
*     - https://stackoverflow.com/questions/32366665/resetting-pointer-to-the-start-of-file/32366729
*     - https://stackoverflow.com/questions/481673/make-a-copy-of-a-char
*     - https://www.geeksforgeeks.org/sum-array-using-pthreads/
*
* 		- As well as files available on blackboard.
*     - Collaborative help from classmates: Alex Rigl and Filip Augustowski
*/

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>

//	============================================================================
//	GLOBAL VARIABLES
//
int nper;	//	Number of hyperperiods
//int taskSet;	//	File for task set
int sched;	//	File for scheduler
int ticks = 0;	//	Number of computer ticks
int curr = 0;	//	current thread to run
int numTasks = 0;
int lcm;

char str[32];	//	String variable
sem_t* sem;	//	semaphore: allow multiple threads to access the shared memory
sem_t* semCon;	//	semaphore than controls the multiple threads semaphore
struct taskProc* t;	//	each task is of type of taskProc


//	============================================================================
//	FUNCTION DECLARATIONS
//

//	task():
//	Input: Argument from thread creation.
//	Output:	Write thread ID to output file when called by scheduler.
void *task(void *vargp);

//	scheduler():
//	Input: Use global variables.
//	Output: Implements Rate-Monotonic Scheduler Algorithm to simulate multiple
//					processes via threads. It will call a thread to write to an output
//					file.
void scheduler();

//  checkSche();
//	Input: None, uses global variables.
//	Output:	Checks if schedulable based off of the formula:
//              C1/P1 + C2/P2 + . . . + Cn/Pn < 1
void checkSche();

//	HELPER METHODS	------------------------------------------------------------

//	Write time row to file
void writeTime();
//	Greatest Common Divisor (needed for LCM)
int gcd(int a, int b);
//	Calculate Least Common Multiple of the tasks' period lengths
int findLCM();
//	Recursive Bubble Sort function to sort the tasks based on period lengths
void bubbleSort(int n);


//	============================================================================
//	STRUCTS
//
typedef struct taskProc{
    char* name;	//	Name of Task
    int wcet;	//	The WCET of task
    int per;	//	The period length of task
    int loc;	// 	The current location, or how much time was applied towards task
    int counter;	//	Counter to keep track of time into the task's period
} taskProc;


//	===========================================================================
//	MAIN
//
int main(int argc, char *argv[])
{
  //	The thread identifiers
	pthread_t* pt;	//	task thread

	//	ERROR CHECK ==============================================================
	//	No Input
	if (argc != 4) {
		printf("Program requires three arguments: ./prodcon <nperiods> <task set> <schedule>\n");
		exit(EXIT_FAILURE);
	}

	//	Ensure first input is positive
	if (atoi(argv[2]) < 0 ) {
		printf("<nperiods> must be a positive.\n");
		exit(EXIT_FAILURE);
	}

	//	FILE / INPUTS ============================================================
	//	Cast user inputs to int and save to global variables
	nper = atoi(argv[1]);	//	grab number of hyperperiods

	sched = creat(argv[3], O_WRONLY | O_APPEND);
	if (sched == -1)
  {
    printf("ERROR: Could not open the second inputted file.\n");
    printf("       Please check if the file exists and is entered correctly.\n");
    exit(EXIT_FAILURE);
  }

	FILE* taskSet;	//	Input file
  if((taskSet = fopen(argv[2], "r")) == NULL)	//	Open the file
  {
    printf("ERROR: Task Set file cannot be opened \n");
    exit(EXIT_FAILURE);
  }

  //  --------------------------------------------------------------------------
  //  Input File Management
	char fl[32];

  char* name = malloc(sizeof(char)*2);	//	Character buffers for file search
	int x = 0;	//	iterator for file
	int wcet, period;	//	temp values for data from file

	//	Count number of tasks in file
	while (fgets(fl, 32, taskSet) != NULL) {
		numTasks++;
	}

	//	Allocate memory of task object for the number of tasks found
	t = malloc(sizeof(taskProc)*numTasks);

	//	Reset file pointer to beginning
	rewind(taskSet);

	//	Read data from each line of file
	while (fgets(fl, 32, taskSet) != NULL) {
		sscanf(fl, "%s %d %d", name, &wcet, &period);

    char* temp = malloc(sizeof(name));
    strcpy(temp, name);

    t[x].name = temp;
		t[x].wcet = wcet;
		t[x].per = period;
		x++;
	}

  //  Close the input file
	fclose(taskSet);


	//	INITIALIZATION ===========================================================
	//	Allocate memory for semaphores
	sem = malloc(sizeof(sem_t)*nper);	//	Size of n hyperperiods, one for each thread
	semCon = malloc(sizeof(sem_t));	//	Separate control semaphore to control the sem
	//	Allocate memory for threads
	pt = malloc(sizeof(pthread_t)*nper);	// Size of n hyperperiods

	//	--------------------------------------------------------------------------
	//	Set all thread semaphores to 0, locked
  int i;
	for (i = 0; i < nper; i++)
	{
		sem_init(&sem[0], 0, 0);
	}
	//	Set control semaphore to 0, locked
	sem_init(semCon, 0 , 0);

	//	Create n threads
	for (i = 0; i < nper; i++)
	{
		int* id = malloc(sizeof(int));	// grabs current i, if code not here it'd be off by 1
		*id = i;
		pthread_create(&pt[i], NULL, task, id);
	}

	//	--------------------------------------------------------------------------
	//	Start the scheduler program
  checkSche();
	scheduler();
	//fflush(stdout);

	//	--------------------------------------------------------------------------
	//	Allow remaining threads to exit
	for (i = 0; i < nper; i++)
			sem_post(&sem[i]);

	//	Join n threads
	for (i = 0; i < nper; i++)
		pthread_join(pt[i], NULL);

	//	CLEANUP ==================================================================
	free(t);	//	Free the thread memory
	free(sem);	//	Free the thread semaphores memory
  close(sched);

	return 0;
}


//	============================================================================
//	THREAD: task
//
//	Input: Argument passed from thread creation. It is the ID number for the thread
//	Output:	Write name of thread to output file when called by scheduler.
//
void *task(void *vargp)
{
	//	Set thread id to passed argument (id from creating the thread)
	int id = *((int *)vargp);

	//	While scheduling is running
	while(curr != -1)
	{
		//	Thread will wait until scheduler releases it
		if(sem_wait(&sem[id]) != 0)
		{
			printf("Semaphore error in thread %d\n", id);
			exit(EXIT_FAILURE);
		}

		//	If scheduler has finished, then don't write to file and exit loop
		if(curr == -1)
			break;

		//	Write name of thread to file
		sprintf(str, "%s ", t[id].name);
		write(sched, str, strlen(str));
    //fflush(stdout);

		//	Release the semaphore controler
		sem_post(semCon);
	}

	free(vargp);	//	Free passed argument memory
	pthread_exit(0);
}

//	============================================================================
//	Input: None, uses global variables.
//	Output:	Checks if schedulable based off of the formula:
//              C1/P1 + C2/P2 + . . . + Cn/Pn < 1
//
void checkSche()
{
  float sum = 0.0;  // sum of each task
  float finalSum = 0.0; //  sum of all the tasks
  int i;
  for(i = 0; i < numTasks; i++)
  {
    sum = ((float)t[i].wcet / (float)t[i].per);
    finalSum += sum;
  }

  //  Abort program if sum is less than 1
  if(finalSum > 1)
  {
    printf("Schedule is not possible.\n");
    free(t);	//	Free the thread memory
  	free(sem);	//	Free the thread semaphores memory
    close(sched); //  Close the schedule file
    exit(EXIT_FAILURE);
  }
}

//	============================================================================
//	Input: None, uses global variables.
//	Output:	Simulate RM Scheduling via threads. Each thread will be called to
//					write out to a file at the time that it was called.
//
void scheduler()
{
	//----------------------------------------------------------------------------
	//	Calculate LCM of tasks' periods
	lcm = findLCM();
	//	Sort the tasks and prioritize by least period time
	bubbleSort(numTasks);
	//	Write time row to file
	writeTime();

	//----------------------------------------------------------------------------
	int cont = 1;	//	allow loop to continue
  int n, i, j, k, l, m;
  for(n = 0; n < nper; n++)
	{
		//	Main loop - Loop until ticks have reached the lcm
		while(ticks != lcm)
		{
			//	Loop through the sorted tasks
			for(i = 0; i < numTasks; i++)
			{
				//	loop through number of remaining ticks until thread's wcet is reached
				for(j = t[i].loc; j < t[i].wcet; j++)
				{
					sem_post(&sem[i]);	//	unlock determined thread
					sem_wait(semCon);	//	wait until thread gives access back to main loop

					//	Iterate clock ticks and current thread's used processing time
					ticks++;
					t[i].loc++;

					//	Iterate all threads' counters and check if any have reach their
					//	period, if so then break out.
					for(k = 0; k < numTasks; k++)
					{
						t[k].counter++;
						if(t[k].counter == t[k].per)
						{
							//	Reset thread's values if it has reached end of its period
							t[k].loc = 0;
							t[k].counter = 0;
							i = -1;
							cont = 0;
						}
					}

					//	If previous loop determined that thread needs to break, then break
					//	next loop level
					if(cont == 0)
					{
						cont = 1;
						break;
					}
				}
			}

			//	Continue iterating counters even if WCET has been reached
			for(l = 0; l < numTasks; l++)
			{
				t[l].counter++;
				if(t[l].counter == t[l].per)
				{
					//	Reset thread's values if it has reached end of its period
					t[l].loc = 0;
					t[l].counter = 0;
				}
			}

			//	Write blank if no thread needs to be called
			write(sched, "-- ", strlen("-- "));
			//fflush(stdout);
			//	Iterate clock ticks
			ticks++;
		}

		//	Reset all thread's values when starting next hyperperiod
		for(m = 0; m < numTasks; m++)
		{
			t[m].loc = 0;
			t[m].counter = 0;
			ticks = 0;
		}
		write(sched, "\n", strlen("\n"));
	}
	write(sched, "\n", strlen("\n"));
	printf("Scheduler has finished successfully.\n");
	curr = -1;	//	Tell threads to exit
}

//  ============================================================================
//	HELPER METHODS
//

//	----------------------------------------------------------------------------
//	Write number of ticks of the hyperperiod at top of file
void writeTime()
{
  int i;
	for (i = 0; i < lcm; i++)
	{
		//	For numbers of two digits, remove one space to make it easier to read
		if(i > 9)
		{
			sprintf(str, "%d ", i);
			write(sched, str, strlen(str));
		}

		else
		{
			sprintf(str, "%d  ", i);
			write(sched, str, strlen(str));
		}
	}
	write(sched, "\n", strlen("\n"));
}

//	----------------------------------------------------------------------------
//	geeksforgeeks.com
//	Function to return gcd of a and b
int gcd(int a, int b)
{
    if (a == 0)
        return b;
    return gcd(b % a, a);
}

//	----------------------------------------------------------------------------
//	geeksforgeeks.com
//	Function to find lcd of array of numbers
int findLCM()
{
    // Initialize result
    int ans = t[0].per;

    // ans contains LCM of arr[0], ..arr[i]
    // after i'th iteration,
    int i;
    for (i = 1; i < numTasks; i++)
        ans = (((t[i].per * ans)) / (gcd(t[i].per, ans)));

    return ans;
}

//	----------------------------------------------------------------------------
//	geeksforgeeks.com
//	Recursive Bubble Sorting Algorithm
void bubbleSort(int n)
{
    // Base case
    if (n == 1)
        return;

    // One pass of bubble sort. After
    // this pass, the largest element
    // is moved (or bubbled) to end.
    int i;
    for (i = 0; i < n - 1; i++)
		{
      if (t[i].per > t[i+1].per)
			{
				struct taskProc temp = t[i];
				t[i] = t[i+1];
				t[i+1] = temp;
			}
		}

    // Largest element is fixed,
    // recur for remaining array
    bubbleSort(n-1);
}
