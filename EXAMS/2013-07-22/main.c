#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
#include <time.h>
#include <string.h>



typedef struct _DATA{

	int nStations;
	int nThreads;

	int direction;
	int station;

	int id;

	sem_t *s_station_forward;
	sem_t *s_station_backward;
	sem_t *s_station_change;



}DATA;


void *work ( void *param);
int main ( int agrc , char *argv[] ){

	srand ( time ( NULL ) );

	int nStations = atoi ( argv[1] );
	int nThreads  = atoi ( argv[2] );

	pthread_t *th = ( pthread_t * ) malloc ( nThreads * sizeof ( pthread_t ));
	
	DATA *data = ( DATA * ) malloc ( nThreads * sizeof ( DATA ));

	sem_t *s_station_forward  = ( sem_t * ) malloc ( nStations * sizeof ( sem_t ));
	sem_t *s_station_backward = ( sem_t * ) malloc ( nStations * sizeof ( sem_t ));
	sem_t *s_station_change   = ( sem_t * ) malloc ( nStations * sizeof ( sem_t ));

	//sem initialization
	for ( int i = 0; i < nStations; i++ ){
		sem_init ( &s_station_forward[i]  , 0 , 1 );
		sem_init ( &s_station_backward[i] , 0 , 1 );
		sem_init ( &s_station_change[i]   , 0 , 1 );
		
		
	}

	for ( int i = 0; i < nThreads; i++ ){
		
		data[i].nStations 	   = nStations;
		data[i].nThreads  	   = nThreads;
		data[i].direction 	   = rand()%2; //[0,1]
		data[i].station 	   = rand()%nStations; 
		data[i].id 		   = i;

		data[i].s_station_forward  = s_station_forward;
		data[i].s_station_backward = s_station_backward;
		data[i].s_station_change   = s_station_change;

		pthread_create ( &th[i] , NULL , work , &data[i] );
	

	}

	for ( int i = 0; i < nThreads; i++)
		pthread_join(th[i] , NULL );

	return 0;
}


void *work ( void *param){


	DATA *data = ( DATA * ) param;

	int nextStation;


	while ( 1 ){

		if ( data->direction == 0 )// forward ( clockwise )
			sem_wait ( &data->s_station_forward[data->station] );

		else if ( data->direction == 1 )// bacward ( countclockwise )
			sem_wait ( &data->s_station_backward[data->station] );

		if ( data->direction == 0 )
			fprintf ( stdout , "Train n. %d in station %d going CLOCKWISE\n" , data->id , data->station  );

		else if ( data->direction == 1 )
			fprintf ( stdout , "Train n. %d in station %d going COUNTCLOCKWISE\n" , data->id , data->station  );

		sleep(rand()%6);

		int app;
		if ( data->direction == 1 ){
			if ( data->station == 0 )
				app = data->nStations - 1;
			else
				app = data->station - 1;
		}
		if ( data->direction == 0 )
			app = data->station;

		//wait a free rail
		sem_wait ( &data->s_station_change[app] );


		if ( data->direction == 0 )// forward ( clockwise )
			sem_post ( &data->s_station_forward[data->station] );

		else if ( data->direction == 1 )// bacward ( countclockwise )
			sem_post ( &data->s_station_backward[data->station] );


		
		if ( data->direction == 0 )
			fprintf ( stdout , "Train n. %d travelling toward station %d\n" , data->id , app++ );
		else if ( data->direction == 1 )
			fprintf ( stdout , "Train n. %d travelling toward station %d\n" , data->id , app-- );

		
	
		sleep(10);

		
		//calculating new data->station value
		if ( data->direction == 0 ){	
			data->station++;
			if ( data->station >= data->nStations  ){
				data->station = 0;
				sem_post(&data->s_station_change[data->nStations - 1]);
			}
			else
				sem_post(&data->s_station_change[data->station]);

		}
		if ( data->direction == 1 ){	
			data->station--;
			if ( data->station  < 0 ){
				data->station  = data->nStations - 1;
				sem_post(&data->s_station_change[0]);

			}
			else
				sem_post(&data->s_station_change[data->station]);
		}

		fprintf ( stdout , "Train n. %d arrived at station station %d\n" , data->id , data->station );


	}


}
