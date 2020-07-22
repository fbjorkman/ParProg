#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#define ARRSIZE 1000000
#define MAXWORKERS 12

int arr[ARRSIZE];
long numberOfThreads = 0;
int size;
int numWorkers;

pthread_attr_t attr;
pthread_t workerid[MAXWORKERS];
pthread_mutex_t lock_create;

struct Thread{
	int lower;
	int upper;
};

int chosePiv(int lower, int upper){

	if(upper-lower < 3)
		return lower;

	int mid;
	mid = (upper+lower)/2;
	if(arr[mid] >= arr[mid-1]){
		if(arr[mid] <= arr[mid+1])
			return mid;
		else{
			if(arr[mid-1] >= arr[mid+1])
				return mid-1;
			else
				return mid+1;
		}
	}
	else{
		if(arr[mid-1] <= arr[mid+1])
			return mid-1;
		else{
			if(arr[mid] >= arr[mid+1])
				return mid;
			else
				return mid+1;
		}
	}
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

void sorted(){
	bool sorted = true;
	int i, count = 0;
	for(i = 0; i < size-1; i++){
		if(arr[i]>arr[i+1]){
			sorted = false;
			count++;
		}
	}
	if(sorted)
		printf("The array is sorted\n");
	else
		printf("The array is not sorted. %d errors\n", count);
}

void insertionSort(void* Thread){
	
	struct Thread* p = (struct Thread*) Thread;
	int lower = p->lower;
	int upper = p->upper;

	int temp, i, j;

	for(i = lower; i <= upper; i++){
		temp = arr[i];
		j = i;
		while(temp < arr[j-1] && j > lower){
			arr[j] = arr[j-1];
			j--;
		}
		arr[j] = temp;
	}
}


void* quicksort(void* Thread){

	struct Thread* p = (struct Thread*) Thread;
	int lower = p->lower;
	int upper = p->upper;

	if(lower < upper){
		struct Thread Thread_0;
		struct Thread Thread_1;
		int piv, temp, swap, i;

		if(upper - lower < 100){
			Thread_0.lower = lower;
			Thread_0.upper = upper;
			insertionSort(&Thread_0);
		}

		piv = chosePiv(lower, upper);
		//printf("piv: %d lower: %d upper: %d\n", arr[piv], lower, upper);

		//swap piv to the farmost right position, the upper positions value becomes the new piv
		temp = arr[upper];
		arr[upper] = arr[piv];
		arr[piv] = temp;

		swap = lower;
		for(i = lower; i < upper; i++){
			if(arr[i] < arr[upper]){
				temp = arr[i];
				arr[i] = arr[swap];
				arr[swap] = temp;
				swap++;
			}
		}

		//move piv to its correct location
		temp = arr[swap];
		arr[swap] = arr[upper];
		arr[upper] = temp;

		/*printf("quicksort: ");
		for(i = lower; i <= upper; i++)
			printf("%d ", arr[i]);
		printf("piv: %d\n", arr[swap]);*/

		//recursive call on each side of the piv

		if(numberOfThreads < numWorkers){
			if(swap - lower > 1){
				Thread_0.lower = lower;
				Thread_0.upper = swap-1;
				pthread_mutex_lock(&lock_create);
				if(numberOfThreads < numWorkers){
					int ok = pthread_create(&workerid[numberOfThreads++], &attr, quicksort, &Thread_0);
					printf("created: %d lower: %d upper: %d\n", ok, Thread_0.lower, Thread_0.upper);
					//printf("id: %ld\n", pthread_self());
					pthread_mutex_unlock(&lock_create);
				}
					
				else{
					pthread_mutex_unlock(&lock_create);
					quicksort(&Thread_0);
				}
			}
		}
		else
			if(swap - lower > 1){
				Thread_0.lower = lower;
				Thread_0.upper = swap-1;
				quicksort(&Thread_0);
			}

		if(upper - swap > 1){
			Thread_1.lower = swap+1;
			Thread_1.upper = upper;
			quicksort(&Thread_1);
		}
	}
}


void main(int argc, char *argv[]){
	int i;
	double start_time, end_time; /* start and end times */

	size = (argc > 1)? atoi(argv[1]) : ARRSIZE;
	numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
	if (size > ARRSIZE) size = ARRSIZE;
	if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
	
	//Init array
	//srand(time(NULL));
	for(i = 0; i < ARRSIZE; i++){
		arr[i] = rand()%1000;
	}

	/*for(i=0;i<ARRSIZE;i++)
		printf("%d ", arr[i]);
	printf("\n");*/

	struct Thread start;
	start.lower = 0;
	start.upper = size-1;

	start_time = read_timer();
	quicksort(&start);
	for(int l = 0; l < numberOfThreads; l++){
    	int ok = pthread_join(workerid[l], NULL);
    	printf("join %d, id: %ld\n", ok, workerid[l]);
  	}

  	/*for(i=0;i<ARRSIZE;i++)
		printf("%d ", arr[i]);
	printf("\n");*/
	
	end_time = read_timer();
	sorted();
	printf("The execution time is %g ms\n", (end_time - start_time)*1000);
}