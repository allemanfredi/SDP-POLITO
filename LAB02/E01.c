#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>

#define NUM_THREADS 5


pthread_t th1 , th2;
sem_t *s;
int tMax = 5;
pid_t pidTh2;

typedef struct ThreadData{
	int id;
	int tMax;
}threadData;

threadData *data;


// return 1 in case of timeout
int wait_with_timeout ( sem_t *s , int tMax ){
	
	int time = 0;

	sleep ( tMax / 1000 );
	if ( sem_trywait ( s ) == 0 ) // if thread2 has sent a message it means that in tmax second it has 
				      // arrived the request		
			return 0; 

	
	return 1;
	

} 

void *th1Func ( void *arg ){

	threadData *d = ( threadData * ) arg;
	int t =  ( ( rand() % 4 ) + 1 ) ;
	sleep ( t / 1000);
	
	//wait no more than tMax milliseconds
	if ( t > tMax  )
		t = tMax;
	
	printf ( "ID : %d waiting on a semaphore after %d milliseconds \n " , d->id , t);
	
	if ( wait_with_timeout ( s , tMax ) == 0 ) 
		printf ( "ID : %d Wait returned normally\n" , d->id );
	else{ // in this case i have to kill the second thread which still sleeping
		printf ( "ID : %d Wait on semaphore s returned for timeout\n" , d->id);
		kill ( pidTh2 , SIGALRM );
	    }


}

void *th2Func ( void *arg ){
	
	pidTh2 = getpid();
	threadData *d = ( threadData * ) arg;
	int t =  ( ( rand() % 1000 ) + 1 ) * 10;
	sleep ( t / 1000 );
	
	printf ( "ID : %d Performing signal on semaphore s after %d milliseconds\n" , d->id , t ); 

	sem_post(s);		

}






int main ( int argc , char *argv[] ){
	
	
	srand ( time ( NULL ) );
	data = ( struct ThreadData * ) malloc ( NUM_THREADS *sizeof ( struct ThreadData ));
    
    tMax = atoi ( argv[1] );
	
	// sempahore initialization
	s = ( sem_t * ) malloc ( sizeof ( sem_t ) );
	if ( sem_init ( s , 0 , 0 ) )
		fprintf ( stderr , "Error during the semaphore initialization" );

	// thread 1 creation
	data[0].id = 1;
	data[0].tMax = tMax;
	pthread_create ( &th1 , NULL , th1Func , ( void * ) &data[0] );

	//thread 2 creation	
	data[1].id = 2;
	data[1].tMax = tMax;
	pthread_create ( &th2 , NULL , th2Func , ( void * ) &data[1] );
	


	pthread_join ( th1 , NULL );
	pthread_join ( th2 , NULL );
	sem_destroy ( s );

	
	return 0;
}




