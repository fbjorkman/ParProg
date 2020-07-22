#include <stdlib.h>
#include <stdio.h>	
#include <mpi.h>
#include <stdbool.h>
#include <time.h>

int numofprocesses;

enum GENDER{MAN, WOMAN};

struct Person{
	enum GENDER gender;
	bool engaged;
	int myid;
};

// A utility function to swap two integers
void swap (int *a, int *b){
    int temp = *a;
    *a = *b;
    *b = temp;
}

// A function to generate a random permutation of arr[]
void shuffle (int arr[], int n, int myid){
	int i;
    // Use a different seed value so that we don't get same
    // result each time we run this program
    srand (time(NULL)*(myid+1));
 
    // Start from the last element and swap one by one. We don't
    // need to run for the first element that's why i > 0
    for (i = n-1; i > 0; i--)
    {
        // Pick a random index from 0 to i
        int j = rand() % (i+1);
 
        // Swap arr[i] with the element at random index
        swap(&arr[i], &arr[j]);
    }
}

int findWoman(int rating[], int size){
	int i, index = 0;
	int bestFit = rating[0];
	for(i = 0; i < size; i++){
		if(rating[i] > bestFit){
			bestFit = rating[i];
			index = i;
		}
	}
	return index+1;
}

void proposing(int rating[], struct Person* p, int size){
	int proposal = p->myid;
	int ans;
	int woman;
	while(true){
		woman = findWoman(rating, size);
		printf("Man %d proposing to woman %d\n", p->myid, woman);
		MPI_Send(&proposal, 1, MPI_INT, woman, 0, MPI_COMM_WORLD);
		MPI_Recv(&ans, 1, MPI_INT, woman, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if(ans == 1){
			printf("Man %d engaged to woman %d rating %d\n", p->myid, woman, rating[woman]);
			MPI_Recv(&ans, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			if(ans == -1)
				return;
			rating[woman-1] = 0;
		}
		else{
			rating[woman-1] = 0;
		}
	}
}

void choosingMan(int rating[], struct Person* p, int size){
	int proposal;
	int man;
	int accept = 1;
	int decline = 0;

	MPI_Recv(&proposal, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	printf("Woman %d recieved proposal from man %d\n", p->myid, proposal);
	MPI_Send(&accept, 1, MPI_INT, proposal, 0, MPI_COMM_WORLD);
	printf("Woman %d accepted proposal from man %d\n", p->myid, proposal);
	man = proposal;
	MPI_Send(&accept, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

	while(true){
		MPI_Recv(&proposal, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if(proposal == -1){
			printf("Woman %d marries man %d\n", p->myid, man);
			return;
		}
		printf("Woman %d recieved proposal from man %d\n", p->myid, proposal);

		if(rating[proposal-(size+1)] > rating[man-(size+1)]){
			MPI_Send(&decline, 1, MPI_INT, man, 0, MPI_COMM_WORLD);
			printf("Woman %d breaks up with man %d\n", p->myid, man);
			MPI_Send(&accept, 1, MPI_INT, proposal, 0, MPI_COMM_WORLD);
			printf("Woman %d accepted proposal from man %d\n", p->myid, proposal);
			man = proposal;
			//MPI_Send(&accept, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		}
		else{
			MPI_Send(&decline, 1, MPI_INT, proposal, 0, MPI_COMM_WORLD);
			printf("Woman %d declines proposal from man %d\n", p->myid, proposal);
		}
	}
}

void controller(){
	int count = 0;
	int add;
	int finished = -1;
	bool done = false;

	while(!done){
		MPI_Recv(&add, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		count++;
		if(count == (numofprocesses-1)/2){
			done = true;
		}
	}
	for(int i = 1; i < numofprocesses; i++){
		MPI_Send(&finished, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
	}
	printf("***************************%d couples***************************\n", count);
}

int main(int argc, char** argv){
	int i;
	int rank;
	int size;
	// Initialize the MPI environment
    MPI_Init(NULL, NULL);
	// Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &numofprocesses);
    // Get the rank of the process (ID)
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    struct Person p;
    p.myid = rank;
    p.engaged = false;
    size = numofprocesses/2;
    int rating[size];

    if(rank == 0){
    	controller();
    }
    else{
	    if(rank < (numofprocesses+1)/2){
			p.gender = WOMAN;
		}
		else{
			p.gender = MAN;
		}

		for(i = 0; i < size; i++){
			rating[i] = i+1;
		}
		shuffle(rating, size, p.myid);

		/*for(i = 0; i < numofprocesses/2; i++)
			printf("%d ", rating[i]);
		printf("\n");*/

		if(p.gender == MAN){
			proposing(rating, &p, size);
		}
		else if(p.gender == WOMAN){
			choosingMan(rating, &p, size);
		}
	}

    // Finalize the MPI environment.
    MPI_Finalize();
    return 0;
}