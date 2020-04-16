/*
* @name: rmsched.c
* @author: Matt Raymond
* @email: raymo116@mail.chapman.edu
* @id: 2270559
* @date: 04/15/2020
* @version: 1.0
*/

#define STR_LENGTH 32

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// A struct to hold all of the data from a process
typedef struct  {
    char* name;
    int wcet;
    int period;
    // The time left in the cpu burst
    int current;
} proc;

// An array for the processes
// Makes them easier to work with
typedef struct {
    proc* p;
    int num;
} proc_holder;

// The thread function. Simply outputs its name to a file
void *threadFun(void*);

// Creates a single proc object
proc* createProcess(proc*, char*, int, int);

// Creates all of the proc objects and stores them in a "vector"
int createProcs(char*, proc_holder*);

// Frees a process's memory
void deleteProcs(proc_holder*);

// Initializes all of the semaphores
void initSem();

// A function that runs the rm scheduling algorithm the given number of times
int runSim(int);

// Checks to see if the given number is an lcm
int checkArray(int);

// Finds the least common multiple
int lcm();

// Finds the max period from the processes
int max();

// Checks to see if the rm scheduler is able to schedule the processes
int checkIfRunable();

// Array of semaphores for processes
sem_t* sem;

// Semaphore for the main simulation
sem_t* mainSem;

// An array of procs to store process data in
struct proc* p;

// An array for the filepath to the task set
char* taskSet;

// Effectively a vector that holds the processes
proc_holder ph;

// Whether the simulation is running or not
// Tells the threads whether to block again or not
int running = 1;

// The number of times for the program to run
int nPeriods;

// The lcm of all of the processes
int l;

// A filepath for where the schedule should be saved
char* scheduler;

// A fp for the output file
FILE* fptr;

// An array of thread IDs
pthread_t* tid;

/*
The idea is to write a C program that simulates a Rate Monotonic (RM) real-
scheduler. The scheduler will create n number of threads (based upon the tasks
defined in the task set file). Then simulate the scheduling of those threads
using posix based semaphores. Each thread will wait in a while loop waiting to
acquire the semaphore. Once acquired the thread will print-out just its task
name then go wait for the next semaphore post by the scheduler (Only one
function should be needed to implement the thread tasks). The scheduler will
release or post a semaphore (so n tasks means n sempahores) based upon the RM
scheduling algorithm. A “clock tick” will be simulated through each iteration of
the main scheduling loop (i.e. one iteration first clock tick, two iterations,
second clock tick,). Assume all task are periodic and released at time 0.
*/
int main(int argc, char *argv[]) {

    // Checks to make sure the input format is correct
    if(argc != 4) {
        printf("Usage: ./rmsched <nperiods> <task set> <schedule>\n");
        return -1;
    }
    else if (atoi(argv[1]) < 1) {
        printf("Argument %s must be positive\n", argv[1]);
        return -1;
    }

    // Assigns the values
    nPeriods = atoi(argv[1]);
    taskSet = argv[2];
    scheduler = argv[3];

    // Creates the processes from the given file
    createProcs(taskSet, &ph);
    // Initializes all the necessary semaphores
    initSem();

    // Creates the list of threads
    tid = malloc(sizeof(pthread_t)*ph.num);
    l = lcm();

    // Checks to see if the processes can even be scheduled
    if(checkIfRunable()<=1) {
        int i;
        // Create the threads
        for(i = 0; i < ph.num; ++i) {
            // Give each one an id
            int* temp = malloc(sizeof(int));
            *temp = i;
            pthread_create(&tid[i],NULL,threadFun,temp);
        }

        // Opens the file
        // Need to fix this
        if((fptr = fopen(scheduler, "w+")) == NULL) {
            printf("You cannot open this file\n");
            return -1;
        }

        // If the simulation crashes, removes the schedule file that was created
        if(runSim(nPeriods) == -1) remove(scheduler);

        // Join all of the threads
        for (i = 0; i < ph.num; ++i) {
            pthread_join(tid[i],NULL);
        }
    }
    else
        printf("This is unable to be scheduled\n");


    // Free memory back up
    deleteProcs(&ph);
    free(tid);
    free(sem);
    return 0;
}

// This takes in an int for the number of hyperperiods and outputs to a file
// that many hyperperiods
int runSim(int times) {
    // A psuedo-stack for organizing our processes
    int* stack = malloc(sizeof(int)*ph.num);
    int top_s= -1;

    // prints number at the top of the page
   	int j;
    for(j = 0; j < l; ++j) {
        fprintf(fptr, "%d  ", j);
    }
    fprintf(fptr, "\n");

    // Does this once for every hyper-period
    // This is slightly inefficient, but works for our purposes
    while(times-->0) {
        // The "world time"
	    int t;
        // Goes for the lcm to form one hyperperiod
        for(t = 0; t < l; ++t) {
            // Cycles through all of processes
            int i;
            for(i = 0; i < ph.num; ++i) {

                // If it's time for the process to start again
                if(t%ph.p[i].period == 0) {
                    // And the process has completed its last burst
                    if(ph.p[i].current == 0) {

						// Add to stack
                        stack[++top_s] = i;
                        // Reset the value of current
                        ph.p[i].current = ph.p[i].wcet;

						// Move the process down the stack until it reaches the
                        // appropriate point
						int k;
						for(k = top_s; k > 0; --k) {
							if(ph.p[stack[k]].period > ph.p[stack[k-1]].period) {
								int temp = stack[k];
						        stack[k] = stack[k-1];
								stack[k-1] = temp;
			    			}
                        }
		    		}
                    // If the process must go again but hasn't finished its last
                    // cpu burst, that means that the scheduler failed
		    		else {
            			printf("These processes cannot be scheduled\n");
                		running = 0;
                		return -1;
                	}
				}
			}

            // If the stack is not empty
			if(top_s != -1) {
                // Have the thread save its name to the file
				sem_post(&sem[stack[top_s]]);
				sem_wait(mainSem);
                // Decrement the cpu burst
				ph.p[stack[top_s]].current--;
                // If the burst is over, remove it from the stack
				if(ph.p[stack[top_s]].current == 0) top_s--;
			}
            // If there's no process in the stack, save a placeholder
			else {
				fprintf(fptr, "__ ");
			}
		}
        // Save a newline
        fprintf(fptr, "\n");
    }

    // Close the file
    fclose(fptr);
    // Stop the threads from continuing
	running = 0;

    // Post all of the threads so that they unblock and exit
	int i;
	for (i = 0; i < ph.num; ++i) {
        sem_post(&sem[i]);
    }

    // Free up the memory used by the stack
    free(stack);
    return 0;
}

// This is the function that the threads perform
// All they do is save their name to a file
void *threadFun(void* param) {
    // The id of the thread
    int id = *((int *) param);

    // While they should be repeating
    while(running) {
        // Block
        sem_wait(&sem[id]);
        // If they should be executing code
        if(running) {
            // Save their name to a file
			fprintf(fptr, "%s ", ph.p[id].name);

            // Post the main function (not "main()")
            sem_post(mainSem);
        }
	}

    // Free up the data used by the parameter
    free(param);
    pthread_exit(0);
}

// Initializes the semaphores
void initSem() {
    // Initializes the main semaphore for the main simulation method
    mainSem = malloc(sizeof(sem_t));
    // Initializes the semaphore
    if(sem_init(mainSem,0,0) == -1) {
            printf("%s\n",strerror(errno));
    }

    // Allocates memory for all of the semaphores
    sem = malloc(sizeof(sem_t)*ph.num);

    // Initializes semaphores for the threads
    int i;
    for (i = 0; i < ph.num; ++i) {
        if(sem_init(&sem[i],0,0) == -1) {
            printf("%s\n",strerror(errno));
        }
    }
}

// Creates a proc object and returns a pointer to it
proc* createProcess(proc* p, char* name, int wcet, int period) {
    p->name = name;
    p->wcet = wcet;
    p->period = period;
    p->current = 0;
    return p;
}

// Creates all of the processes by reading from a file
int createProcs(char* fp, proc_holder* ph) {
    // File pointer
    FILE* fptr;

    // Error handling for if the file doesn't open
    if((fptr = fopen(fp, "r+")) == NULL) {
        printf("That file doesn't exist\n");
        return -1;
    }

    // Temp variables to count how many processes are in the file
    char tempName[STR_LENGTH];
    int wcet, period;

    // Gets the number of lines with processes on them
    ph->num = 0;
    int ret;
    while(1) {
        ret = fscanf(fptr, "%s %d %d", tempName, &wcet, &period);
        if(ret == 3)
            ++ph->num;
        else if(errno != 0) {
            perror("scanf:");
            break;
        } else if(ret == EOF) {
            break;
        } else {
            printf("No match.\n");
        }
    }

    // Go back to the begining of the file
    rewind(fptr);

    // Allocate the necessary memory for the array of procs
    ph->p = malloc(sizeof(proc)*ph->num);
    ph->num=0;

    // Cycles through the file again, but this time creates procs
    while(1) {
        char* n = malloc(sizeof(char)*STR_LENGTH);
        ret = fscanf(fptr, "%s %d %d", n, &wcet, &period);
        if(ret == 3)
            createProcess(&ph->p[ph->num++], n, wcet, period);
        else if(errno != 0) {
            perror("scanf:");
            break;
        } else if(ret == EOF) {
            break;
        } else {
            printf("No match.\n");
        }
    }

    // Closes the file
    fclose(fptr);
    return 0;
}

// Frees memory from the proc datastructures on completion
void deleteProcs(proc_holder* ph) {
    // Frees proc names
    int i;
    for(i = 0; i < ph->num; ++i) {
        free(ph->p[i].name);
    }
    // Frees the array of procs
    free(ph->p);
}

// Checks to see if the array is divisible by the given number
int checkArray(int lcm) {
    // Checks against every period in the proc_holder
    int i;
    for(i = 0; i < ph.num; ++i) {
        if(lcm%(ph.p[i].period != 0)) return 0;
    }
    return 1;
}

// Generates the lcm and returns it
int lcm() {
    int l = max();
    while(checkArray(l) != 1) ++l;
    return l;
}

// Finds the max period in the proc_holder
int max() {
    int max = 0;
    int i;
    for(i = 0; i < ph.num; ++i) {
        if(ph.p[i].period > max) max = ph.p[i].period;
    }
    return max;
}

// Checks to see if the processes are even schedulable
int checkIfRunable() {
    int sum = 0;
    int i;
    for(i = 0; i < ph.num; ++i) {
        sum += ph.p[i].wcet/ph.p[i].period;
    }
    return sum;
}
