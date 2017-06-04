#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>

typedef struct ThreadData{
	int coloumn;
	int row;
} ThreadData;

ThreadData *data;
pthread_t *thread , p_th;
pthread_mutex_t mutex;
sem_t *sem , *print_result;

int **m1 , **m2 , **mr;
int nr1 , nc1 , nr2 , nc2;
int count = 0;


void *func ( void *arg ){
	
	ThreadData *d = ( struct ThreadData * ) arg;
	
	sem_wait ( sem );
	pthread_mutex_lock ( &mutex );
	
	for ( int j = 0; j < nr2; j++ ){

	 	mr[d->row][d->coloumn] += m1[d->row][j] * m2[j][d->coloumn];
		
	}
	count++;
		
	if ( count == ( nr1 * nc2 ) ) sem_post ( print_result );
	pthread_mutex_unlock ( &mutex );


}


void *print ( void *arg ) {
	
	sem_wait ( print_result );
	for ( int i = 0; i < nr1 ; i++ ){
		for ( int j = 0; j < nc2; j++ )
			printf ( "%d " , mr[i][j] );
		printf ( "\n" );
	}
	

}

int main ( int argc , char *argv[] ) {
		
	
	nr1 = atoi ( argv[1] );
	nc1 = atoi ( argv[2] );
	nr2 = atoi ( argv[3] );
	nc2 = atoi ( argv[4] );
	printf ( "ROW1 %d COLOUMN1 %d ROW2 %d COLOUMN2 %d\n " , nr1 , nc1 , nr2 , nc2);

	if ( nc1 != nr2 ) { printf ( "error matrix dimension\n" ); return 0; }

	// dynamic allocate 
	m1 = ( int ** ) malloc ( nr1 * sizeof ( int * ) );
	for ( int i = 0; i < nr1; i++ )
		m1[i] = ( int * ) malloc ( nc1 * sizeof ( int ) );

	m2 = ( int ** ) malloc ( nr2 * sizeof ( int * ) );
	for ( int i = 0; i < nr2; i++ )
		m2[i] = ( int * ) malloc ( nc2 * sizeof ( int ) );
	
	mr = ( int ** ) malloc ( nr1 * sizeof ( int * ) );
	for ( int i = 0; i < nr1; i++ )
		mr[i] = ( int * ) malloc ( nc2 * sizeof ( int ) );
	
			
	// matrix filling
	int num = 1;
	for ( int i =0; i < nr1; i++ )
		for ( int j = 0; j < nc1; j++ , num++ )
			m1[i][j] = num;

	num = 1;
	for ( int i =0; i < nr2; i++ )
		for ( int j = 0; j < nc2; j++ , num++ )
			m2[i][j] = num;


	for ( int i =0; i < nr1; i++ )
		for ( int j = 0; j < nc2; j++  )
			mr[i][j] = 0;

	// struct allocation
	data = ( struct ThreadData * ) malloc ( ( nr1 * nc2  ) * sizeof ( struct ThreadData ) );
		
	
	// struct filling ( 0-0 , 0-1 , .....  nr1-nc2 )
	int row = 0, coloumn = 0;
	int i = 0;
	while ( i < ( nr1 * nc2 ) ){
		while ( coloumn < nc2 ){
			data[i].row = row;			
			data[i].coloumn = coloumn;
			coloumn++;
			i++;
		}
		coloumn=0;
		row++;
	}	
	
	//semaphore initialization and creation
	sem = ( sem_t *) malloc ( sizeof ( sem_t ) );
	print_result = ( sem_t * ) malloc ( sizeof ( sem_t ) );	
	sem_init ( sem , 0 , nr1 * nc2 );
	sem_init ( print_result , 0 , 0 );
	
	
	//mutex initialization
	pthread_mutex_init ( &mutex , NULL );

	//thread creation and initialization
	thread = ( pthread_t * ) malloc ( ( nc1 * nr2 ) * sizeof ( pthread_t ) ); 

	for ( int i = 0; i < ( nr1 * nc2 ); i++ )
		pthread_create (&thread[i] , NULL , func , ( void * ) &data[i] );

	// thread for printing creatin
	pthread_create ( &p_th , NULL , print , NULL );
	
	pthread_join ( p_th , NULL );
	
	
	return 0;
}
