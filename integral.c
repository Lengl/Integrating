#include <semaphore.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <malloc.h>
#include <stdlib.h>
#include "stack.h"
#include <sys/time.h>

#define LOC_KRIT 10

//command line: number of threads, left border, right border
//to compile: gcc -o integral integral.c -lpthread -lrt -lm

//Shared memory:
stack glbStack;
int nActive = 0;
double iglSum = 0;
sem_t sem_glbStack, sem_iglSum, sem_taskPresence;
double eps = 0.0000000001;
long long nSegm = 0;
unsigned int NTHR = 0;


double f(double x) {
	return sin(1.0/x) * sin(1.0/x) / (x * x);
}

void recieveTask(record *tempTask) {
	//wait for task in global stack
	sem_wait(&sem_taskPresence);
	//close global stack
	sem_wait(&sem_glbStack);
	//get record from global stack
	//here we don't have & because we already sent a pointer into function
	getFromStack(&glbStack, tempTask);
	//say to others if there are more in stack
	if (glbStack.stTop)
		sem_post(&sem_taskPresence);
	//if it is not terminating record increase number of active processes
	if (tempTask->A <= tempTask->B)
		nActive++;
	//open global stack
	sem_post(&sem_glbStack);
}

void integrateTask(record tmpTask, double *locSum, long long *locNum) {
	stack locStack;
	stackInit(&locStack);
	while(1) {
		double C = (tmpTask.A + tmpTask.B) / 2;
		double fC = f(C);
		double sAC = (tmpTask.fA + fC) * (C - tmpTask.A) / 2;
		double sCB = (tmpTask.fB + fC) * (tmpTask.B - C) / 2;
		double sACB = sAC + sCB;
// 		printRecord(tmpTask);
		
		if(fabs(tmpTask.sAB - sACB) <= eps) {
			//calculation for AB finished
			(*locSum) += sACB;
			(*locNum)++;
			//if there are no more segments in local stack - integration finished
			if(locStack.stTop == 0) {
// 				fprintf(stdout, "localstack empty\n");
				break;
			}
			//else
			getFromStack(&locStack, &tmpTask);
		} else {
			//put right segment into local stack, left into tempTask
			record locPut = {.A = tmpTask.A, .B = C, .fA = tmpTask.fA, .fB = fC, .sAB = sAC};
// 			fprintf(stdout, "locput: "); printRecord(locPut);
			putIntoStack(&locStack, &locPut);
			tmpTask.A = C;
			tmpTask.fA = fC;
			tmpTask.sAB = sCB;
		}
		//here we already have job in tmpTask
		//so we can throw extra work in global stack
		sem_wait(&sem_glbStack);
		//if we have too much job and there is no job in global stack
		if((locStack.stTop > LOC_KRIT) && !(glbStack.stTop)) {
			while((locStack.stTop > 1) && (glbStack.stTop < STACK_MAX_TASK)){
				//put one from top of the local stack, decrease amount of stacks in local stack
				putIntoStack(&glbStack, &(locStack.stk[--locStack.stTop]));
			}
			sem_post(&sem_taskPresence);
		}
		sem_post(&sem_glbStack);
	}
}

void * slaveThread(void * none) {
	//initialize local variables
	double locSum = 0;
	long long locNum = 0;
	record tempTask;
	
	while(1) {
		recieveTask(&tempTask);
		//if it is terminating record - break cycle
		if (tempTask.A > tempTask.B) {
// 			printf("I WAS HERE!!\n");
			break;
		}
		//start integrating cycle for task from global stack
		integrateTask(tempTask, &locSum, &locNum);
		//integrating task from global stack ended - we are not active anymore
		sem_wait(&sem_glbStack);
		nActive--;
		//if we were the last one counting - there are no more active and no tasks in global stack
		if(!(nActive) && !(glbStack.stTop)) {
			//put NTHR terminators in global stack
			record terminator = {.A = 2, .B = 1, .fA = 0, .fB = 0, .sAB = 0};
			int i;
			for(i = 0; i < NTHR; i++) {
				putIntoStack(&glbStack, &terminator);
			}
			sem_post(&sem_taskPresence);
		}
		sem_post(&sem_glbStack);
	}
	//we are terminated - add our sum to global one and exit
	sem_wait(&sem_iglSum);
// 	fprintf(stdout, "my locSum = %lf\n", locSum);
	iglSum += locSum;
	nSegm += locNum;
	sem_post(&sem_iglSum);
	return;
}

int main(int argc, char *argv[]) {
	struct timeval start, end;
	long long startusec, endusec;
	double elapsed;
	//Initialize global variables
	double myA = atof(argv[2]), myB = atof(argv[3]);
	stackInit(&glbStack);
	record startSegment = {.A = myA, .B = myB, .fA = f(myA), .fB = f(myB), .sAB = (f(myA) + f(myB)) * (myB - myA) / 2};
	printRecord(startSegment);
	putIntoStack(&glbStack, &startSegment);
	sem_init(&sem_taskPresence, 0, 1);
	sem_init(&sem_glbStack, 0, 1);
	sem_init(&sem_iglSum, 0, 1);
	
	gettimeofday(&start, NULL);
	//start NTHR processes
	NTHR = atoi(argv[1]);
	pthread_t *tid = (pthread_t *)malloc(NTHR * sizeof(pthread_t));
	int i;
	for(i = 0; i < NTHR; i++) {
		int err;
		err = pthread_create(&(tid[i]), NULL, slaveThread, NULL);
		if (err != 0) {
			fprintf(stderr, "Mistakes were made: code %d\n", err);
			exit(1);
		}
	}
	
	for(i = 0; i < NTHR; i++) {
		int err;
		err = pthread_join(tid[i], NULL);
		if (err != 0) {
			fprintf(stderr, "Mistakes were made: code %d\n", err);
			exit(1);
		}
	}
	
	gettimeofday(&end, NULL);
	startusec = start.tv_sec * 1000000 + start.tv_usec;
	endusec = end.tv_sec* 1000000 + end.tv_usec;
	elapsed = (double)(endusec - startusec )/1000000.0;
	
	//output the result
	fprintf(stdout, "integral value = %.10f\naccuracy = %.10f\ntime = %lf, nSegm = %ld\n", iglSum, eps * nSegm, elapsed, nSegm);
	
	//clean-up
	free(tid);
	sem_destroy(&sem_taskPresence);
	sem_destroy(&sem_iglSum);
	sem_destroy(&sem_glbStack);
}