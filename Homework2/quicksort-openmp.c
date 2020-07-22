#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <omp.h>
#define ARRSIZE 100000
#define MAXWORKERS 4

int arr[ARRSIZE];
int numWorkers;


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
	for(i = 0; i < ARRSIZE-1; i++){
		if(arr[i]>arr[i+1])
			sorted = false;
	}
	if(sorted)
		printf("The array is sorted\n");
	else
		printf("The array is not sorted\n");
}

void insertionSort(int lower, int upper){
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


void quicksort(int lower, int upper){

	if(lower < upper){
		int piv, temp, swap, i;
		if(upper - swap < 100)
			insertionSort(swap+1, upper);

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
	
		//recursive call on each side of the piv
#pragma omp task
			if(swap - lower > 1){
				//if(swap - lower < 100)
				//	insertionSort(lower, swap-1);
				//else
					quicksort(lower, swap-1);
			}

			if(upper - swap > 1){
			//	if(upper - swap < 100)
			//		insertionSort(swap+1, upper);
			//	else
					quicksort(swap+1, upper);
			}
	}
}


void main(int argc, char *argv[]){
	int i;
	double start_time, end_time; /* start and end times */

	numWorkers = (argc > 1)? atoi(argv[1]) : MAXWORKERS;
	if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
	
	//Init array
	srand(time(NULL));
	for(i = 0; i < ARRSIZE; i++){
		arr[i] = rand()%1000;
	}

	start_time = read_timer();
	omp_set_num_threads(numWorkers);
#pragma omp parallel
#pragma omp single nowait
	quicksort(0, ARRSIZE-1);
	end_time = read_timer();
	sorted();
	printf("The execution time is %g ms\n", (end_time - start_time)*1000);
}