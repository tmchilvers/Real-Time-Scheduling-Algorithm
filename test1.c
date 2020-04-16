#define NUMBER 3
#define STR_LENGTH 32

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <string.h>

typedef struct  {
    char* name;
    int wcet;
    int period;
    int current;
} proc;

typedef struct {
    proc* p;
    int num;
} proc_holder;

void *threadFun(void*);

proc* createProcess(proc*, char*, int, int);
void deleteProcs(proc_holder*);
int createProcs(char*, proc_holder*);
void printProc(proc_holder*);
void initSem();
int runSim(int);
int checkArray(int);
int lcm();
int max();
int checkIfRunable();

// Threading
sem_t* sem;
sem_t* mainSem;
struct proc* p;
char* taskSet;
proc_holder ph;
int running = 1;
int nPeriods;
int l;
int active;
char* scheduler;
FILE* fptr;

/*
The producer/consumer program (prodcon.c) that takes three arguments from the
command line (no prompting the user from within the program).
*/
int main(int argc, char *argv[]) {
    pthread_t* tid;

    // Input
    if(argc != 4) {
        printf("Usage: ./rmsched <nperiods> <task set> <schedule>\n");
        return -1;
    }
    else if (atoi(argv[1]) < 1) {
        printf("Argument %s must be positive\n", argv[1]);
        return -1;
    }

    nPeriods = atoi(argv[1]);
    taskSet = strdup(argv[2]);
    scheduler = strdup(argv[3]);

    createProcs(taskSet, &ph);
    initSem();

    tid = malloc(sizeof(pthread_t)*ph.num);
    l = lcm();

    if(checkIfRunable()<=1) {
        for (int i = 0; i < ph.num; ++i) {
            int* temp = malloc(sizeof(int));
            *temp = i;
            pthread_create(&tid[i],NULL,threadFun,temp);
        }

        if((fptr = fopen(strdup(scheduler), "w+")) == NULL) {
            printf("You cannot open this file\n");
            return -1;
        }

        if(runSim(nPeriods) == -1) remove(scheduler);
        for (int i = 0; i < ph.num; ++i) {
            sem_post(&sem[i]);
        }

        for (int i = 0; i < ph.num; ++i) {
            pthread_join(tid[i],NULL);
        }
    }
    else
        printf("This is unable to be scheduled\n");


    deleteProcs(&ph);
    free(tid);
    free(sem);
    return 0;
}

int runSim(int times) {

    int* stack = malloc(sizeof(int)*ph.num);
    int top_s= -1;
    for(int i = 0; i < l; ++i) {
        fprintf(fptr, "%d  ", i);
    }
    fprintf(fptr, "\n");

    while(times-->0) {
        for(int t = 0; t < l; ++t) {
            for(int i = 0; i < ph.num; ++i) {
                if(t%ph.p[i].period == 0) {

                    if(ph.p[i].current == 0) {
                        stack[++top_s] = i;
                        ph.p[i].current = ph.p[i].wcet;

                        for(int k = top_s; k > 0; --k) {
                            if(ph.p[stack[k]].period > ph.p[stack[k-1]].period) {
                                int temp = stack[k-1];
                                stack[k-1] = stack[k];
                                stack[k] = temp;
                            }
                        }
                    }
                    else {
                        printf("These processes cannot be scheduled\n");
                        running = 0;
                        return -1;
                    }
                }
            }
            if(top_s != -1) {
                // fprintf(fptr, "%d:", t);
                fflush(stdout);
                sem_post(&sem[stack[top_s]]);
                sem_wait(mainSem);
                if (ph.p[stack[top_s]].current == 0) --top_s;
            }
            else {
                // fprintf(fptr, "%d: __ ", t);
                fprintf(fptr, "__ ");
                fflush(stdout);
            }
        }
        fprintf(fptr, "\n");
    }

    fclose(fptr);

    running = 0;
    free(stack);
    return 0;
}

void *threadFun(void* param) {
    int id = *((int *) param);
    while(running) {
        sem_wait(&sem[id]);
        if(running) {
            --ph.p[id].current;
            fprintf(fptr, "%s ", ph.p[id].name);
            fflush(stdout);
            sem_post(mainSem);
        }
    }

    free(param);
    pthread_exit(0);
}

void initSem() {
    mainSem = malloc(sizeof(sem_t));
    if(sem_init(mainSem,0,0) == -1) {
            printf("%s\n",strerror(errno));
    }

    sem = malloc(sizeof(sem_t)*ph.num);
    for (int i = 0; i < ph.num; ++i) {
        if(sem_init(&sem[0],0,0) == -1) {
            printf("%s\n",strerror(errno));
        }
    }
}

proc* createProcess(proc* p, char* name, int wcet, int period) {
    p->name = name;
    p->wcet = wcet;
    p->period = period;
    p->current = 0;
    return p;
}

int createProcs(char* fp, proc_holder* ph) {
    FILE* fptr;

    if((fptr = fopen(strdup(fp), "r+")) == NULL) {
        printf("That file doesn't exist\n");
        return -1;
    }

    char tempName[STR_LENGTH];
    int wcet, period;

    // gets the number of lines
    // Try to reduce later
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

    rewind(fptr);
    ph->p = malloc(sizeof(proc)*ph->num);
    ph->num=0;

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

    fclose(fptr);
    return 0;
}

void deleteProcs(proc_holder* ph) {
    for(int i = 0; i < ph->num; ++i) {
        free(ph->p[i].name);
    }
    free(ph->p);
}

void printProc(proc_holder* ph) {
    for(int i = 0; i < ph->num; ++i) {
        printf("%s %d %d\n", ph->p[i].name, ph->p[i].wcet, ph->p[i].period);
    }
}

int checkArray(int lcm) {
    for(int i = 0; i < ph.num; ++i) {
        if(lcm%(ph.p[i].period != 0)) return 0;
    }
    return 1;
}

int lcm() {
    int l = max();
    while(checkArray(l) != 1) ++l;
    return l;
}

int max() {
    int max = 0;
    for(int i = 0; i < ph.num; ++i) {
        if(ph.p[i].period > max) max = ph.p[i].period;
    }
    return max;
}

int checkIfRunable() {
    int sum = 0;
    for(int i = 0; i < ph.num; ++i) {
        sum += ph.p[i].wcet/ph.p[i].period;
    }
    return sum;
}
