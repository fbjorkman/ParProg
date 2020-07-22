/* matrix summation using pthreads

   features: uses a barrier; the Worker[0] computes
             the total sum from partial sums computed by Workers
             and prints the total sum to the standard output

   usage under Linux:
     gcc matrixSum.c -lpthread
     a.out size numWorkers

*/
#ifndef _REENTRANT 
#define _REENTRANT 
#endif 
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 10   /* maximum number of workers */
//#define DEBUG

pthread_mutex_t barrier;  /* mutex lock for the barrier */
pthread_cond_t go;        /* condition variable for leaving */
pthread_mutex_t lock_max;
pthread_mutex_t lock_min;
pthread_mutex_t lock_sum;
pthread_mutex_t lock_rowcount;
int numWorkers;           /* number of workers */ 
int numArrived = 0;       /* number who have arrived */
int max = 0;
int min = INT_MAX;
int maxPos[2];
int minPos[2];
int totalSum = 0;
int rowcount = 0;

void changeMax(int maxComp, int pos[]){
  pthread_mutex_lock(&lock_max);
  if(maxComp > max){
    max = maxComp;
    maxPos[0] = pos[0];
    maxPos[1] = pos[1];
  }
  pthread_mutex_unlock(&lock_max);
}

void changeMin(int minComp, int pos[]){
  pthread_mutex_lock(&lock_min);
  if(minComp < min){
    min = minComp;
    minPos[0] = pos[0];
    minPos[1] = pos[1];
  }
  pthread_mutex_unlock(&lock_min);
}

void changeSum(int add){
  pthread_mutex_lock(&lock_sum);
  totalSum += add;
  pthread_mutex_unlock(&lock_sum);
}

int incrementRowcount(){
  pthread_mutex_lock(&lock_rowcount);
  return rowcount++;
}

/* a reusable counter barrier */
void Barrier() {
  pthread_mutex_lock(&barrier);
  numArrived++;
  if (numArrived == numWorkers) {
    numArrived = 0;
    pthread_cond_broadcast(&go);
  } else
    pthread_cond_wait(&go, &barrier);
  pthread_mutex_unlock(&barrier);
}

/* timer */
double read_timer() {
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;
    if( !initialized )
    {
        gettimeofday( &start, NULL );
        initialized = true;
    }
    gettimeofday( &end, NULL );
    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

double start_time, end_time; /* start and end times */
int size, stripSize;  /* assume size is multiple of numWorkers */
int sums[MAXWORKERS]; /* partial sums */
int matrix[MAXSIZE][MAXSIZE]; /* matrix */

void *Worker(void *);

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
  int i, j;
  long l; /* use long in case of a 64-bit system */
  pthread_attr_t attr;
  pthread_t workerid[MAXWORKERS];

  /* set global thread attributes */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  /* initialize mutex and condition variable */
  pthread_mutex_init(&barrier, NULL);
  pthread_cond_init(&go, NULL);

  /* read command line args if any */
  size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
  if (size > MAXSIZE) size = MAXSIZE;
  if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
  stripSize = size/numWorkers;

  srand(time(NULL));
  /* initialize the matrix */
  for (i = 0; i < size; i++) {
	  for (j = 0; j < size; j++) {
          matrix[i][j] = rand()%99;
	  }
  }

  /* print the matrix */
#ifdef DEBUG
  for (i = 0; i < size; i++) {
	  printf("[ ");
	  for (j = 0; j < size; j++) {
	    printf(" %d", matrix[i][j]);
	  }
	  printf(" ]\n");
  }
#endif

  /* do the parallel work: create the workers */
  start_time = read_timer();
  for (l = 0; l < numWorkers; l++){
    pthread_create(&workerid[l], &attr, Worker, (void *) l);
  }
  //wait for all threads to finish
  for(l = 0; l < numWorkers; l++){
    pthread_join(workerid[l], NULL);
  }


  /* get end time */
  end_time = read_timer();
  /* print results */
  printf("The total is %d\n", totalSum);
  printf("The maximum value is %d and the position is (%d, %d)\n", max, maxPos[0], maxPos[1]);
  printf("The minimum value is %d and the position is (%d, %d)\n", min, minPos[0], minPos[1]);
  printf("The execution time is %g sec\n", end_time - start_time);
  pthread_exit(NULL);
}

/* Each worker sums the values in one strip of the matrix.
   After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg) {
  long myid = (long) arg;
  int total, i, j, currentrow;
  int localMax, localMin;
  int lmaxPos[2], lminPos[2];

#ifdef DEBUG
  printf("worker %d (pthread id %d) has started\n", myid, pthread_self());
#endif

  //Set first value and position as min and max
  localMax = 0;
  localMin = INT_MAX;

  while(rowcount < MAXSIZE){
    currentrow = incrementRowcount();
    pthread_mutex_unlock(&lock_rowcount);

    if(currentrow >= MAXSIZE)
      return 0;

    /* sum values in my strip and calculate local max and min*/
    total = 0;
    for (j = 0; j < MAXSIZE; j++){
      total += matrix[currentrow][j];

      if(localMax < matrix[currentrow][j]){
        localMax = matrix[currentrow][j];
        lmaxPos[0] = currentrow;
        lmaxPos[1] = j;
      }

      if(localMin > matrix[currentrow][j]){
        localMin = matrix[currentrow][j];
        lminPos[0] = currentrow;
        lminPos[1] = j;
      }      
    }
  }
  
  if(max < localMax)
    changeMax(localMax, lmaxPos);
  if(min > localMin)
    changeMin(localMin, lminPos);

  changeSum(total);
  
}
