#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <queue>
#define VCAP 10
#define NumberOfV 500
#define MFL 20			//Maximum Fuelrequest Limit
#define TRIPS 5

int Vcap;
int freeSpots;
long Vammount;
int fcount = 0;
std::queue <int> fuelingSpot;

pthread_cond_t freeSpot;
pthread_cond_t Nrefill;
pthread_cond_t Qrefill;
pthread_cond_t refueled;
pthread_cond_t prioRefueled;
pthread_mutex_t lock_spots;
pthread_mutex_t lock_fuel;
pthread_mutex_t lock_Nrefuel;
pthread_mutex_t lock_Qrefuel;

class SpaceStation{
private:
	int Ncap;
	int Qcap;
	int V;
	int N;
	int Q;

public:
	SpaceStation(int V, int N, int Q){
		this->V = V;
		this->N = N;
		this->Q = Q;
		Ncap = N;
		Qcap = Q; 
	}

	int getSpot(){
		int spot;
		if(freeSpots > 0){
			pthread_mutex_lock(&lock_spots);
			if(freeSpots > 0){
				freeSpots--;
				spot = fuelingSpot.front();
				fuelingSpot.pop();
				pthread_mutex_unlock(&lock_spots);
			}
			else{
				pthread_cond_wait(&freeSpot, &lock_spots);
				freeSpots--;
				spot = fuelingSpot.front();
				fuelingSpot.pop();
				pthread_mutex_unlock(&lock_spots);
			}
		}
		else{
			pthread_cond_wait(&freeSpot, &lock_spots);
			freeSpots--;
			spot = fuelingSpot.front();
			fuelingSpot.pop();
			pthread_mutex_unlock(&lock_spots);
		}
		return spot;
	}

	void fueling(int Nreq, int Qreq, int spot){
		N -= Nreq;
		Q -= Qreq;
		if(N < MFL*2){pthread_cond_broadcast(&Nrefill);}
		if(Q < MFL*2){pthread_cond_broadcast(&Qrefill);}
		pthread_mutex_unlock(&lock_fuel);
		//sleeps between 10000-20000 microsec to imitate the time it takes to refuel
		usleep(100000+rand()%100000);
		pthread_mutex_lock(&lock_spots);
		printf("%d %d %d\n", N, Q, ++fcount);
		freeSpots++;
		fuelingSpot.push(spot);
		pthread_mutex_unlock(&lock_spots);
		assert(N >= 0 && Q >= 0);
		pthread_cond_broadcast(&freeSpot);
	}

	void arriving(int Nreq, int Qreq, int id){
		bool fueled = false;
		int spot;
		pthread_mutex_lock(&lock_fuel);
		while(!fueled){
			spot = getSpot();
			if((Nreq+MFL) > N || (Qreq+MFL) > Q){
				pthread_mutex_lock(&lock_spots);
				freeSpots++;
				fuelingSpot.push(spot);
				pthread_mutex_unlock(&lock_spots);
				pthread_cond_broadcast(&freeSpot);
				pthread_cond_wait(&refueled, &lock_fuel);
			}
			else{
				printf("SV nr %d fueling at spot %d\n", id, spot);
				fueling(Nreq, Qreq, spot);
				fueled = true;	
			}	
		}
	}

	void refueling(int Nrefuel, int Qrefuel, int Nreq, int Qreq){
		int spot = getSpot();
		pthread_mutex_lock(&lock_fuel);
		N += Nrefuel;
		Q += Qrefuel;
		printf("refueled %d %d\n", N, Q);
		fueling(Nreq, Qreq, spot);
		pthread_cond_broadcast(&prioRefueled);
		pthread_cond_broadcast(&refueled);
	}

	int getV(){return V;}
	int getN(){return N;}
	int getQ(){return Q;}
	int getNcap(){return Ncap;}
	int getQcap(){return Qcap;}
	void decreaseN(int Namount){this->N -= Namount;}
	void decreaseQ(int Qamount){this->Q -= Qamount;}
	void increaseN(int Namount){this->N += Namount;}
	void increaseQ(int Qamount){this->Q += Qamount;}
};

class SpaceVehicle{
private:
	int Nreq;
	int Qreq;

public:
	SpaceVehicle(int Nreq, int Qreq){
		this->Nreq = Nreq;
		this->Qreq = Qreq;
	}
	int getNreq(){return Nreq;}
	int getQreq(){return Qreq;}
};

class SupplyVehicle{
private:
	int Nrefuel;
	int Qrefuel;
	int Nreq;
	int Qreq;

public:
	SupplyVehicle(int Nrefuel, int Qrefuel, int Nreq, int Qreq){
		this->Nrefuel = Nrefuel;
		this->Qrefuel = Qrefuel;
		this->Nreq = Nreq;
		this->Qreq = Qreq;
	}
	int getNrefuel(){return Nrefuel;}
	int getQrefuel(){return Qrefuel;} 
	int getNreq(){return Nreq;}
	int getQreq(){return Qreq;}
};

void *Fueling(void *);
void *NRefueling(void *);
void *QRefueling(void *);
SpaceStation SS(Vcap, 500, 500);

int main(){
	Vcap = VCAP;
	Vammount = NumberOfV;
	freeSpots = Vcap;

	for(int i = 0; i < Vcap; i++){
		fuelingSpot.push(i+1);
	}

	/* use long in case of a 64-bit system */
	pthread_attr_t attr;
	pthread_t workerid[Vammount+2];
	/* set global thread attributes */
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	pthread_cond_init(&freeSpot, NULL);
	pthread_cond_init(&Nrefill, NULL);
	pthread_cond_init(&Qrefill, NULL);
	pthread_cond_init(&refueled, NULL);
	pthread_cond_init(&prioRefueled, NULL);
	pthread_mutex_init(&lock_spots, NULL);
	pthread_mutex_init(&lock_fuel, NULL);
	pthread_mutex_init(&lock_Nrefuel, NULL);
	pthread_mutex_init(&lock_Qrefuel, NULL);

  	//create the thread that will run the refueling vehicles
	pthread_create(&workerid[Vammount], &attr, NRefueling, (void *) Vammount);
	pthread_create(&workerid[Vammount+1], &attr, QRefueling, (void *) (Vammount+1));
	//create the threads that will run the fueling vehicles
	for (long l = 0; l < Vammount; l++){
		pthread_create(&workerid[l], &attr, Fueling, (void *) l);
	}

  	pthread_exit(NULL);
}

void *Fueling(void *arg){
	long myid = (long) arg;
	int trips = 0;
	//printf("created %ld\n", myid);
	SpaceVehicle SV(rand()%MFL, rand()%MFL);
	while(trips++ < TRIPS){
		SS.arriving(SV.getNreq(), SV.getQreq(), myid);
		//wating 10000000-20000000 microseconds (10-20sec) between each vehicle refuel
		usleep(10000000+rand()%10000000);
	}
}

void *NRefueling(void *arg){
	long myid = (long) arg;
	//printf("supply %ld\n", myid);
	SupplyVehicle SV(450, 0, rand()%MFL, rand()%MFL);
	while(1){
		pthread_cond_wait(&Nrefill, &lock_Nrefuel);
		pthread_mutex_unlock(&lock_Nrefuel);
		SS.refueling(SV.getNrefuel(), SV.getQrefuel(), SV.getNreq(), SV.getQreq());
	}
}

void *QRefueling(void *arg){
	long myid = (long) arg;
	//printf("supply %ld\n", myid);
	SupplyVehicle SV(0, 450, rand()%MFL, rand()%MFL);
	while(1){
		pthread_cond_wait(&Qrefill, &lock_Qrefuel);
		pthread_mutex_unlock(&lock_Qrefuel);
		SS.refueling(SV.getNrefuel(), SV.getQrefuel(), SV.getNreq(), SV.getQreq());
	}
}