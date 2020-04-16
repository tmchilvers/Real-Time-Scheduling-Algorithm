/*
* Programmed by Tristan Chilvers
* ID: 2288893
* Email: chilvers@chapman.edu
*
*	References:
*			- https://www.geeksforgeeks.org/lcm-of-given-array-elements/
*			- https://www.geeksforgeeks.org/recursive-bubble-sort/
* 		- As well as all example files available on blackboard.
*
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
int taskSet;	//	File for task set
int sched;	//	File for scheduler
int ticks = 0;	//	Number of computer ticks
int curr = 0;	//	current thread to run
int numTasks = 3;
int lcm;

char str[32];	//	String variable
sem_t* sem;	//	semaphore: allow multiple threads to access the shared memory
sem_t* semCon;	//	semaphore than controls the multiple threads semaphore
struct taskProc* t;	//	each task is of type of taskProc
int* prior;

//	============================================================================
//	FUNCTION DECLARATIONS
//

//	task:
//	Input: None
//	Output:	None.
void *task(void *vargp);

//	scheduler:
//	Input: None.
//	Output: None.
void scheduler();

//	HELPER METHODS	------------------------------------------------------------
int gcd(int a, int b);
int findLCM();
void bubbleSort(int n);

//	============================================================================
//	STRUCTS
//
typedef struct taskProc{
    char* name;	//	Name of Task
    int wcet;	//	The WCET of task
    int per;	//	The period length of task
    int loc;	// 	The current location (how much time has passed) within task
    int counter;
} taskProc;


//	============================================================================
//	MAIN
//
int main(int argc, char *argv[])
{
	//	The thread identifiers
	pthread_t* pt;	//	task thread

	prior = malloc(sizeof(int)*numTasks);
	t = malloc(sizeof(taskProc)*numTasks);
	t[0].name = "T1";
	t[0].wcet = 2;
	t[0].per = 6;

	t[1].name = "T2";
	t[1].wcet = 3;
	t[1].per = 12;

	t[2].name = "T3";
	t[2].wcet = 6;
	t[2].per = 24;

	for(int i = 0; i < numTasks; i++)
	{
		prior[i] = t[i].per;
	}

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
	//	Cast user inputs to int and save to global variables ---------------------
	nper = atoi(argv[1]);	//	grab size of memory
	taskSet = open(argv[2], O_RDONLY);
	if (taskSet == -1)
	{
	printf("ERROR: Could not open the first inputted file.\n");
	printf("       Please check if the file exists and is entered correctly.\n");
	exit(EXIT_FAILURE);
	}

	sched = creat(argv[3], O_WRONLY | O_APPEND);
	if (sched == -1)
  {
    printf("ERROR: Could not open the second inputted file.\n");
    printf("       Please check if the file exists and is entered correctly.\n");
    exit(EXIT_FAILURE);
  }

	//	INITIALIZATION ===========================================================
	//	Allocate memory for semaphores
	sem = malloc(sizeof(sem_t)*nper);	//	Size of n hyperperiods, one for each thread
	semCon = malloc(sizeof(sem_t));	//	Separate control semaphore to control the sem
	//	Allocate memory for threads
	pt = malloc(sizeof(pthread_t)*nper);	// Size of n hyperperiods

	//	Set all thread semaphores to 0, locked
	for (int i = 0; i < nper; i++)
	{
		sem_init(&sem[0], 0, 0);
	}
	//	Set control semaphore to 0, lockeds
	sem_init(semCon, 0 , 0);

	//	Create n threads
	for (int i = 0; i < nper; i++)
	{
		int* id = malloc(sizeof(int));	// grabs current i, if code not here it'd be off by 1
		*id = i;
		pthread_create(&pt[i], NULL, task, id);
	}

	//	Start the scheduler program
	scheduler();
	fflush(stdout);

	//	Allow remaining threads to exit
	for (int i = 0; i < nper; i++)
	{
			sem_post(&sem[i]);
	}

	printf("joining threads\n");
	//	Join n threads
	for (int i = 0; i < nper; i++)
	{
		pthread_join(pt[i], NULL);
	}


	//	CLEANUP ==================================================================
	free(t);	//	Free the thread memory
	free(sem);	//	Free the thread semaphores memory

	return 0;
}

//	============================================================================
//	THREAD: task
//
//	Input: None
//	Output:	None.
//
void *task(void *vargp)
{
	int id = *((int *)vargp);	//	pass id from

	while(curr != -1)
	{
		if(sem_wait(&sem[id]) != 0)
		{
			printf("Semaphore error in thread\n");
			exit(EXIT_FAILURE);
		}

		if(curr == -1)
		{
			break;
		}

		printf("This is the %d thread speaking!\n\n", id);

		fflush(stdout);
		sem_post(semCon);
	}
	free(vargp);	//	Free passed argument memory
	pthread_exit(0);
}

void scheduler()
{
	//	Calculate LCM of tasks' periods
	lcm = findLCM();
	//	Sort the tasks and prioritize by least period time
	bubbleSort(numTasks);


	//	Write number of ticks of the hyperperiod at top of file
	for (int i = 0; i < lcm; i++)
	{
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


	int cont = 1;
	//int counter = 0;
	//int currTask = prior[0].wcet;
	//	Main loop - Loop until ticks have reached the lcm
	while(ticks != lcm)
	{
		//	Loop through the sorted tasks
		for(int i = 0; i < numTasks; i++)
		{
			//	loop through number of remaining ticks until wcet is reached
			for(int j = t[i].loc; j < t[i].wcet; j++)
			{
				sprintf(str, "%s ", t[i].name);
				write(sched, str, strlen(str));
				fflush(stdout);
				ticks++;
				t[i].loc++;

				for(int k = 0; k < numTasks; k++)
				{
					t[k].counter++;
					//printf("Task %s counter is %d time: %d\n", t[k].name, t[k].counter, ticks);
					if(t[k].counter == t[k].per)
					{
						//printf("Task %s reached its period at time: %d\n", t[k].name, ticks);
						t[k].loc = 0;
						t[k].counter = 0;
						i = -1;
						cont = 0;
						//break;
					}
				}

				if(cont == 0)
				{
					cont = 1;
					break;
				}
			}
		}

		for(int k = 0; k < numTasks; k++)
		{
			t[k].counter++;
			if(t[k].counter == t[k].per)
			{
				t[k].loc = 0;
				t[k].counter = 0;
				//i = -1;
			}
		}
		//sprintf(str, "%s ");
		write(sched, "--", strlen(str));
		fflush(stdout);
		ticks++;

		//sem_post(&sem[curr]);	//	unlock curr thread
		//sem_wait(semCon);	//	wait until thread gives main loop access

		//printf("TICKS: %d\n", ticks);
	}
	write(sched, "\n", strlen(str));
	printf("scheduler says quit\n");
	curr = -1;
}

//  ============================================================================
//	HELPER METHODS
//

//	geeksforgeeks.com
//	Function to return gcd of a and b
int gcd(int a, int b)
{
    if (a == 0)
        return b;
    return gcd(b % a, a);
}

//	geeksforgeeks.com
//	Function to find lcd of array of numbers
int findLCM()
{
    // Initialize result
    int ans = t[0].per;

    // ans contains LCM of arr[0], ..arr[i]
    // after i'th iteration,
    for (int i = 1; i < numTasks; i++)
        ans = (((t[i].per * ans)) / (gcd(t[i].per, ans)));

    return ans;
}

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
    for (int i = 0; i < n - 1; i++)
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
