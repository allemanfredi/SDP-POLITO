#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

pthread_t th_1 , th_2 ;
pthread_mutex_t mutex;

int n = 4;

typedef struct ThreadData{
	int id;
	char type;  // A OR B
} ThreadData;

ThreadData data1 , data2;
sem_t *print1 , *print2 , *acapo;
int stat1 = 0;
int stat2 = 0;

void * proc1 ( void *arg ){
	
	ThreadData *d = ( ThreadData * ) arg;
	
	while ( stat1 * 2 < n ){
	
		sem_wait ( print1 );
		pthread_mutex_lock ( &mutex);
		stat1++;
		//printf (" stat1 %d" , stat1 );
		if ( sem_trywait(acapo) == 0  ) { printf ( "\n"); } 

		if ( stat1 == 1 && stat2 == 1 ) { printf ( "A%d ", d->id ); sem_post(print1); sem_post(print2); }	
		if ( stat1 == 1 && stat2 == 0 ) { printf ( "A%d ", d->id ); sem_post(print2); }
		if ( stat1 == 2 && stat2 == 1 ) { printf ( "B%d " ,d->id ); sem_post(print2); }
		if ( stat2 == 2 && stat1 == 2 ) { printf ( "B%d " ,d->id ); sem_post(acapo); }
		pthread_mutex_unlock ( &mutex );

	}
	



}

void * proc2 ( void *arg ){

	ThreadData *d = ( ThreadData * ) arg;
	
	while ( stat2 * 2 < n ){
	
		sem_wait ( print2 );
		pthread_mutex_lock ( &mutex);
		stat2++;
		//printf (" stat2 %d" , stat2 );
		if ( sem_trywait(acapo) == 0  ) { printf ( "\n"); } 

		if ( stat2 == 1 && stat1 == 1 ) { printf ( "A%d ", d->id ); sem_post(print1); sem_post(print2); }	
		if ( stat2 == 1 && stat1 == 0 ) { printf ( "A%d ", d->id ); sem_post(print1); }
		if ( stat2 == 2 && stat1 == 1 ) { printf ( "B%d " ,d->id ); sem_post(print1); }
		if ( stat2 == 2 && stat1 == 2 ) { printf ( "B%d " ,d->id ); sem_post(acapo); }
		pthread_mutex_unlock ( &mutex );

	}
	
	
	
	


}

int main ( int argc , char *agrv[] ){
	
    n = atoi(argv[1]);
	// semaphore allocation
	print1 = ( sem_t * ) malloc ( sizeof ( sem_t ) );
	print2 = ( sem_t * ) malloc ( sizeof ( sem_t ) );
	acapo = ( sem_t * ) malloc ( sizeof ( sem_t ) );	
	
	sem_init ( print1 , 0 , 1 ); 
	sem_init ( print2 , 0 , 1 );
	sem_init ( acapo , 0 , 0 ); 


	//mutex inti
	pthread_mutex_init(&mutex,NULL);

	data1.id = 1;
	data1.type = 'A';
	
	data2.id = 2;
	data2.type = 'B';
	
	pthread_create ( &th_1 , NULL , proc1 , &data1 ); 
	pthread_create ( &th_2 , NULL , proc2 , &data2 );
	

	
	pthread_join ( th_1 , NULL );
	pthread_join ( th_2 , NULL );

	
	printf ( "\n");
	

	return 0;
}
