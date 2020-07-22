#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <pthread.h>
#define ARRSIZE 100000
#define MAXWORKERS 4

int arr[ARRSIZE];
long numberOfThreads = 0;

pthread_attr_t attr;
pthread_t workerid[MAXWORKERS];
pthread_mutex_t lock_create;

struct Thread{
	int lower;
	int upper;
	void* l;
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
	int i;
	int count = 0;
	for(i = 0; i < ARRSIZE-1; i++){
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

void *insertionSort(void* Thread){

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

	/*printf("insertionssort: ");
		for(i=lower; i<=upper;i++)
			printf("%d ", arr[i]);
		printf("lower: %d, upper: %d\n", lower, upper);*/
}

void *quicksort(void* Thread){

	struct Thread* p = (struct Thread*) Thread;

	int lower = p->lower;
	int upper = p->upper;

	if(lower < upper){
		int piv, temp, swap, i;
		struct Thread Thread;
		long l;

		if(upper - lower < 5){
			Thread.lower = lower;
			Thread.upper = upper;
			insertionSort(&Thread);
		}

		piv = chosePiv(lower, upper);

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
		for(i=lower; i<=upper;i++)
			printf("%d ", arr[i]);
		printf("piv: %d lower: %d upper: %d\n",swap, lower, upper );*/

		//recursive call on each side of the piv 
		if(swap - lower > 1){
			Thread.lower = lower;
			Thread.upper = swap-1;
			int ok;
			if(numberOfThreads < MAXWORKERS){
				pthread_mutex_lock(&lock_create);
				if(numberOfThreads < MAXWORKERS){
					pthread_mutex_unlock(&lock_create);
					ok = pthread_create(&workerid[numberOfThreads++], &attr, quicksort, &Thread);
					printf("pthread create: %d\n", ok);
				}
				else{
					pthread_mutex_unlock(&lock_create);
					quicksort(&Thread);
				}
					
			}
			else
				quicksort(&Thread);
		}

		if(upper - swap > 1){
			Thread.lower = swap+1;
			Thread.upper = upper;
			/*int ok;
			if(numberOfThreads < MAXWORKERS){
				pthread_mutex_lock(&lock_create);
				if(numberOfThreads < MAXWORKERS){
					pthread_mutex_unlock(&lock_create);
					pthread_create(&workerid[numberOfThreads++], &attr, quicksort, &Thread);
					ok = pthread_create(&workerid[numberOfThreads++], &attr, quicksort, &Thread);
					printf("pthread create: %d\n", ok);
				}
				else{
					pthread_mutex_unlock(&lock_create);
					quicksort(&Thread);
				}*/	
			//}
			//else
				quicksort(&Thread);
		}
	}
}

void main(){
	int i;
	double start_time, end_time; /* start and end times */
	long l;
	
	/* set global thread attributes */
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	//Init array
	srand(time(NULL));
	for(i = 0; i < ARRSIZE; i++){
		arr[i] = rand()%1000;
	}

	struct Thread start;
	start.lower = 0;
	start.upper = ARRSIZE-1;

	/*for(i=0;i<ARRSIZE;i++)
		printf("%d ", arr[i]);
	printf("\n");*/

	start_time = read_timer();
	quicksort(&start);

	//wait for all threads to finish
  	for(l = 0; l < MAXWORKERS; l++){
    	pthread_join(workerid[l], NULL);
  	}

	end_time = read_timer();

	/*for(i=0;i<ARRSIZE;i++)
		printf("%d ", arr[i]);
	printf("\n");*/

	sorted();
	printf("The execution time is %g ms\n", (end_time - start_time)*1000);
}
