/*
* Programmed by Tristan Chilvers
* ID: 2288893
* Email: chilvers@chapman.edu
*
*	References:
*			-
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

//	----------------------------------------------------------------------------
//	GLOBAL VARIABLES
//
int nper;	//	Number of hyperperiods
int taskSet;	//	File for task set
int sched;	//	File for scheduler
int ticks = 0;	//	Number of computer ticks
int curr = 0;	//	current thread to run

char str[32];	//	String variable
sem_t* sem;	//	semaphore: allow multiple threads to access the shared memory
sem_t* semCon;	//

//	----------------------------------------------------------------------------
//	FUNCTION DECLARATIONS
//

//	task:
//	Input: None
//	Output:	None.
void *task(void *vargp);

void scheduler();

//	============================================================================
//	MAIN
//
int main(int argc, char *argv[])
{
	//	The thread identifiers
	pthread_t* t;	//	task thread

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

	//	Write number of ticks of the hyperperiod at top of file
	for (int i = 0; i < nper; i++)
	{
		sprintf(str, "%d  ", i);
		write(sched, str, strlen(str));
	}
	write(sched, "\n", strlen("\n"));

	//	INITIALIZATION ===========================================================
	//	Allocate memory for semaphores
	sem = malloc(sizeof(sem_t)*nper);	//	Size of n hyperperiods, one for each thread
	semCon = malloc(sizeof(sem_t));	//	Separate control semaphore to control the sem
	//	Allocate memory for threads
	t = malloc(sizeof(pthread_t)*nper);	// Size of n hyperperiods

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
		pthread_create(&t[i], NULL, task, id);
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
		pthread_join(t[i], NULL);
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
		//printf("Thread curr: %d\n", curr);
		printf("This is the %d thread speaking!\n\n", id);
		//sleep(1);
		//ticks++;
		//printf("TICKS: %d\n\n", ticks);
		fflush(stdout);
		sem_post(semCon);
	}
	free(vargp);	//	Free passed argument memory
	pthread_exit(0);
}

void scheduler()
{
	while(curr != nper)
	{
		sem_post(&sem[curr]);
		sem_wait(semCon);
		curr++;
	}
	printf("scheduler says quit\n");
	curr = -1;
}
