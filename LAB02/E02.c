#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

pthread_t *th_A , *th_B;
pthread_mutex_t mutex , mutex2;

int nThread = 1000;

typedef struct ThreadData{
	int id;
	char type;  // A OR B
} ThreadData;

ThreadData *dataA , *dataB;
sem_t *printA , *printB;
int cnt_3 = 0;
int statB = 0;
int statA = 0;
int lock = 0;

void * procA ( void *arg ){
	
	ThreadData *d = ( ThreadData * ) arg;
	
	sem_wait( printA );
	pthread_mutex_lock ( &mutex );
	printf ( "%c%d ", d->type , d->id );
	
	statA++;
	
	if ( statA + statB >= 3 ){
		printf ( "\n");

		statA = 0;
		statB = 0;

		sem_post ( printB );
		sem_post ( printA );
		sem_post ( printB );
	}
	pthread_mutex_unlock ( &mutex );


}

void * procB ( void *arg ){

	ThreadData *d = ( ThreadData * ) arg;
	
	
	sem_wait ( printB );
	pthread_mutex_lock ( &mutex );
	printf ( "%c%d ", d->type , d->id );
	
	statB++;

	if ( statA + statB >= 3 ){
		printf ( "\n");

		statA = 0;
		statB = 0;

		sem_post ( printA );
		sem_post ( printB );
		sem_post ( printB );
	}
	pthread_mutex_unlock ( &mutex );
	
	
	


}

int main ( int argc , char *agrv[] ){
	
    nThread = atoi(argv[1]);

	//data allocation
	dataA = ( struct ThreadData * ) malloc ( nThread * sizeof (  struct ThreadData ) ) ;
	dataB = ( struct ThreadData * ) malloc ( 2 * nThread * sizeof ( struct ThreadData ) ) ; 
	
	// thread allocation
	th_A = ( pthread_t * ) malloc ( nThread * sizeof ( pthread_t ) );
	th_B = ( pthread_t * ) malloc ( 2 * nThread * sizeof ( pthread_t ) );

	// semaphore allocation
	printA = ( sem_t * ) malloc ( sizeof ( sem_t ) );
	printB = ( sem_t * ) malloc ( sizeof ( sem_t ) );

	sem_init ( printA , 0 , 1 ); 
	sem_init ( printB , 0 , 2 ); 


	//mutex inti
	pthread_mutex_init(&mutex,NULL);
	pthread_mutex_init(&mutex2,NULL);

	
	
	for ( int i = 0; i < 2 * nThread; i++ ){
		dataB[i].id = i;
		dataB[i].type = 'B';
		pthread_create (&th_B[i] , NULL , procB , ( void * ) &dataB[i] );

	}
	
	for ( int i = 0; i < nThread; i++ ){
		dataA[i].id = i;
		dataA[i].type = 'A';
		pthread_create ( &th_A[i] , NULL , procA , ( void * ) &dataA[i] );

	}
	

	



	for ( int i = 0; i < nThread; i++ ){
		pthread_join ( th_A[i] , NULL );
	}
	for ( int i = 0; i < nThread * 2; i++ ){
		pthread_join ( th_B[i] , NULL );
	}
	printf ( "\n");
	

	return 0;
}
