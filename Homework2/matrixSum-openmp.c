/* matrix summation using OpenMP

   usage with gcc (version 4.2 or higher required):
     gcc -O -fopenmp -o matrixSum-openmp matrixSum-openmp.c 
     ./matrixSum-openmp size numWorkers

*/

#include <omp.h>

double start_time, end_time;

#include <stdio.h>
#include <stdlib.h>
#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 8   /* maximum number of workers */

int numWorkers;
int size; 
int matrix[MAXSIZE][MAXSIZE];
void *Worker(void *);

struct Pos{
	int i;
	int j;
};

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
  int i, j, total=0;
  int max, min;

  /* read command line args if any */
  size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
  if (size > MAXSIZE) size = MAXSIZE;
  if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;

  omp_set_num_threads(numWorkers);

  /*int id = omp_get_thread_num();
  printf("id %d\n", id);*/

  /* initialize the matrix */
  for (i = 0; i < size; i++) {
    //  printf("[ ");
	  for (j = 0; j < size; j++) {
      matrix[i][j] = rand()%99;
      //	  printf(" %d", matrix[i][j]);
	  }
	  //	  printf(" ]\n");
  }
int n;
//for(n=0;n<5;n++){
  max = matrix[0][0];
  min = matrix[0][0];

  struct Pos maxPos = {0, 0};
  struct Pos minPos = {0, 0};

  start_time = omp_get_wtime();
#pragma omp parallel for reduction (+:total) reduction (max:max) reduction (min:min) private(j)
  for (i = 0; i < size; i++)
    for (j = 0; j < size; j++){
      total += matrix[i][j];
      if(matrix[i][j] > max){
      	max = matrix[i][j];
      	maxPos.i = i;
      	maxPos.j = j;
      }
      if(matrix[i][j] < min){
      	min = matrix[i][j];
      	minPos.i = i;
      	minPos.j = j;
      }
    }
// implicit barrier

  end_time = omp_get_wtime();

  /*id = omp_get_thread_num();
  printf("id %d\n", id);*/

  printf("the total is %d\n", total);
  printf("The maxium value is %d on position (%d, %d)\n", max, maxPos.i, maxPos.j);
  printf("The minimum value is %d on position (%d, %d)\n", min, minPos.i, minPos.j);
  printf("it took %g seconds\n", end_time - start_time);
  printf("%g\n", end_time - start_time);
//}

}

