#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>


#define		T_TRAIN		5
#define 	T_MAX		10
#define 	T_CAPACITY	100
#define		N_STATION	4
#define		N_TRAIN		2
#define		T_NEW_PASS	1
#define		L		20
#define		BUFFER		512



typedef struct _PASSENGER{

	int     id;
	int	starting_station;
	int 	arrival_station;

	struct _QUEUE   *queue;

	sem_t		*empty;
	sem_t		*full;
	sem_t           *meP;
	sem_t		*sem_get_off;



}PASSENGER;




typedef struct _QUEUE{

	PASSENGER  	buffer[L];
	int		size;
	int		read;
	int		write;


}QUEUE;


typedef struct _TRAIN{

	int		train;
	int		capacity;
	int 		station;
	int		busy_sit;
	
	QUEUE		*queue_passenger_wait;
	QUEUE		*queue_passenger_on_train;

	sem_t		*red_semaphore;
	sem_t		*empty;
	sem_t		*meC;
	sem_t		*full;

	int		*num_passenger_per_station;

	int		**pipe_station;


}TRAIN;



typedef struct _STATION{

	int		station;
	QUEUE		*queue;
	sem_t		*red_semaphore;

	int		**pipe_station;


}STATION;





void *train_work ( void * param );
void *station_work ( void * param );
void *passenger_work ( void *param );

void init_queue ( QUEUE *q , int size );
PASSENGER *removee ( QUEUE *ptr );
void insert ( QUEUE *ptr , PASSENGER value );
void visit ( QUEUE *ptr );


int main ( int argc , char *argv[] ){

	srand ( time ( NULL ) );

	pthread_t	*t_train , *t_station , *t_passenger;
	
	
	
	//thread allocation
	t_train = ( pthread_t * ) malloc ( N_TRAIN * sizeof ( pthread_t ) );
	t_station = ( pthread_t * ) malloc ( N_STATION * sizeof ( pthread_t ) );
	t_passenger = ( pthread_t * ) malloc ( L * sizeof (  pthread_t ) );



	//structure allocation
	STATION		*d_station   =    ( STATION *) malloc ( N_STATION * sizeof ( STATION ) );
	TRAIN		*d_train     =    ( TRAIN * )  malloc ( N_TRAIN  * sizeof ( TRAIN ) );
	PASSENGER	*d_passenger =    ( PASSENGER * ) malloc ( L * sizeof ( PASSENGER ) );


	//queue allocation	
	QUEUE	*queue_wating_passenger   = ( QUEUE * ) malloc ( N_STATION * sizeof ( QUEUE ) );
	QUEUE	*queue_passenger_on_train = ( QUEUE * ) malloc ( N_STATION * sizeof ( QUEUE ) );

	init_queue ( queue_wating_passenger  , L );
	init_queue ( queue_passenger_on_train , T_CAPACITY );
	

	//P&C initialization
	sem_t	*full  = ( sem_t * ) malloc ( N_STATION * sizeof ( sem_t ) );
	sem_t	*empty = ( sem_t * ) malloc ( N_STATION * sizeof ( sem_t ) );
	sem_t	*meP   = ( sem_t * ) malloc ( N_STATION * sizeof ( sem_t ) );
	sem_t	*meC   = ( sem_t * ) malloc ( N_STATION * sizeof ( sem_t ) );
	
	for ( int i = 0; i < N_STATION; i++ ){
		sem_init ( &full[i] ,  0 , 0 );
		sem_init ( &empty[i] , 0 , L );
		sem_init ( &meP[i] ,   0 , 1 );
		sem_init ( &meC[i] ,   0 , 1 );
	}	
	

	//pipe initialization
	int **pipe_station = ( int ** ) malloc ( N_STATION * sizeof ( int * ) );
	for ( int i = 0; i < N_STATION; i++ )
		pipe_station[i] = ( int * ) malloc ( 2 * sizeof ( int ) );


	for ( int i = 0; i < N_STATION; i++ )
		pipe ( pipe_station[i] );




	//TRAIN 
	sem_t	*red_semaphore = ( sem_t * ) malloc ( N_STATION * sizeof ( sem_t ) );
	for ( int i = 0; i < N_STATION; i++ )
		sem_init ( &red_semaphore[i] , 0 , 1 );

	int *num_passenger_per_station = ( int * ) malloc ( N_STATION * sizeof ( int ) );
	

	for ( int i = 0; i < N_TRAIN; i++ ){
	
		d_train[i].red_semaphore = red_semaphore;
		d_train[i].train = i;
		d_train[i].capacity = T_CAPACITY;
		d_train[i].station  = i; //provisional...can reach only 0 and 1 
		d_train[i].queue_passenger_wait = queue_wating_passenger;
		d_train[i].queue_passenger_on_train = queue_passenger_on_train;
		d_train[i].busy_sit = 0;
		d_train[i].full = full;
		d_train[i].empty = empty;
		d_train[i].meC   = meC;
		d_train[i].num_passenger_per_station = num_passenger_per_station;
		d_train[i].pipe_station = pipe_station;

		printf ( "TRAIN %d START FROM %d\n" , d_train[i].train , d_train[i].station );

		pthread_create ( &t_train[i] , NULL , train_work , &d_train[i] );
	}


	//STATION
	for ( int i = 0; i < N_STATION; i++ ){
		
		d_station[i].red_semaphore = &red_semaphore[i];
		d_station[i].station = i;
		d_station[i].pipe_station = pipe_station;
	

		pthread_create ( &t_station[i] , NULL , station_work , &d_station[i] );
	}



	//PASSENGER
	sem_t   *sem_get_off = ( sem_t * ) malloc ( L * sizeof ( sem_t ) );
	for ( int i = 0; i < L; i++ )
		sem_init ( &sem_get_off[i] , 0 , 0 );

	for ( int i = 0; i < L; i++ ){

		sleep ( T_NEW_PASS );

		d_passenger[i].starting_station = rand() % N_STATION;
		while ( ( d_passenger[i].arrival_station = rand() % N_STATION ) == d_passenger[i].starting_station ); 

		d_passenger[i].queue = queue_wating_passenger;
		d_passenger[i].id = i;	
		d_passenger[i].sem_get_off = &sem_get_off[i]; 
		d_passenger[i].empty = empty;
		d_passenger[i].full  = full;
		d_passenger[i].meP   = meP;


		pthread_create ( &t_passenger[i] , NULL , passenger_work , &d_passenger[i] );
	}

	for ( int i = 0; i < N_TRAIN; i++ )
		pthread_join ( t_passenger[i] , NULL );

	for ( int i = 0; i < N_TRAIN; i++ )
		pthread_join ( t_train[i] , NULL);


	for ( int i = 0; i < N_TRAIN; i++ )
		pthread_join ( t_station[i] , NULL);
	

	return 0;
}



void *train_work ( void * param ){

	TRAIN *t = ( TRAIN * ) param;
	int 		busy_sit;	
	int 		free_sit;
	char 		buf[BUFFER];
	char 		buf_app[BUFFER];

	

	while ( 1 ){

		//only one train for each station
		sem_wait ( &t->red_semaphore[t->station] );

		fprintf ( stdout , "TRAIN %d IS ARRIVED IN STATION %d WITH CAPACITY %d\n" , t->train , t->station , t->capacity - t->busy_sit );

		//here i have to get off the passenger
		free_sit = 0;
		for ( int i = 0; i < t->num_passenger_per_station[t->station]; i++ ){

			//get the passenger on the train in order to get off on the train
			//visit ( &t->queue_passenger_on_train[t->station] );
			PASSENGER *p = removee ( &t->queue_passenger_on_train[t->station] );

			sem_post ( p->sem_get_off );
			printf ( "PASSENGER GET OFF %d\n" , p->id );

			free_sit++;
		}
		t->num_passenger_per_station[t->station] = 0;
		t->capacity += free_sit;

		//load passenger that are wating in order to get on them
		busy_sit = 0;
		for ( int i = 0; i < t->capacity; i++){


			struct timespec ts;
			if ( clock_gettime ( CLOCK_REALTIME , &ts ) == -1 )
					fprintf( stderr , "Error during clock get time \n");
			ts.tv_sec += T_MAX;
	
			int res = sem_timedwait ( &t->full[t->station] , &ts);
			if ( res == - 1 ){	
				if ( errno = ETIMEDOUT )
					fprintf(stdout , "TIMEOUT TRAIN %d\n" , t->train );
				break;
			}
			
				sem_wait ( &t->meC[t->station] );
					PASSENGER *p = removee ( &t->queue_passenger_wait[t->station] );
					insert ( &t->queue_passenger_on_train[p->arrival_station] , *p );
					//visit ( &t->queue_passenger_on_train[p->arrival_station] );
				sem_post ( &t->meC[t->station] );
			sem_post ( &t->empty[t->station ] );
		
			t->num_passenger_per_station[p->arrival_station]++;

			busy_sit++;


		}
		t->capacity -= busy_sit;



		//append data to file
		int fd = open ( "log.txt" , O_APPEND | O_CREAT | O_WRONLY , 0777 );
		
		memset ( buf , 0 , BUFFER);
		memset ( buf , 0 , BUFFER );
		sprintf ( buf , "%d %d " , t->train , t->station );
		for ( int pos = t->queue_passenger_on_train->write - t->queue_passenger_on_train->read; pos < t->queue_passenger_on_train->write; pos++ ){
			sprintf ( buf_app , "%d" , t->queue_passenger_on_train->buffer[pos].id );
			strcat ( buf , " " );
			strcat ( buf , buf_app );
		}

		write ( fd , buf , strlen ( buf ) );
		write ( fd , "\n" , 1 );
		close ( fd );


		//comunication with the staton throught pipe
		write ( t->pipe_station[t->station][1] , &t->train , sizeof ( int ) ); 
		write ( t->pipe_station[t->station][1] , &busy_sit , sizeof ( int ) );
		write ( t->pipe_station[t->station][1] , &free_sit , sizeof ( int ) );


		//leave the station
		sem_post ( &t->red_semaphore[t->station] );

		
		t->station++;
		if ( t->station >= N_STATION - 1 ) 
			t->station = 0;

	}

	

	return NULL;
}

void *passenger_work ( void *param ){

	PASSENGER *p = ( PASSENGER * ) param;
	

	printf ( "PASSENGER %d START FROM %d TO %d\n" , p->id , p->starting_station , p->arrival_station );	

	//producer and consumer 
	sem_wait ( &p->empty[p->starting_station] );
		sem_wait ( &p->meP[p->starting_station] );
			insert (  &p->queue[p->starting_station] , *p );
			//visit (  &p->queue[p->starting_station] );
		sem_post ( &p->meP[p->starting_station] );
	sem_post (  &p->full[p->starting_station] );

	
	sem_wait ( p->sem_get_off );
	printf ( "PASSENGER %d HAS ARRIVED \n",  p->id );
}




void *station_work ( void * param ){

	STATION *s = ( STATION * ) param;
	int			train_id;
	int			busy_sit;
	int			free_sit;
	
	while ( 1 ){
		
		read ( s->pipe_station[s->station][0] , &train_id , sizeof ( int ) ); 
		read ( s->pipe_station[s->station][0] , &busy_sit , sizeof ( int ) );
		read ( s->pipe_station[s->station][0] , &free_sit , sizeof ( int ) );

		fprintf ( stdout , "TRAIN %d HAS ARRIVED TO STATION %d ---- PASSENGER THAT HAVE REACHED THEIR DESTINATION %d AND PASSENGER ON TRAIN %d\n" , train_id , s->station , busy_sit , free_sit );


	}

	return NULL;
}



void init_queue ( QUEUE *ptr , int size ){
	
	ptr->size   = size;
	ptr->write  = 0;
	ptr->read   = 0;

	return;
}


PASSENGER *removee ( QUEUE *ptr ){

	PASSENGER	*val;

	val = &ptr->buffer[ptr->read];
	ptr->read = (ptr->read + 1 ) % L;

	return val;
}


void insert ( QUEUE *ptr , PASSENGER value ){

	ptr->buffer[ptr->write] = value;
	ptr->write = (ptr->write + 1 ) % L;

	return;
}


void visit ( QUEUE *ptr ){

	for ( int pos = 0; pos < ptr->write; pos++ )
		printf ( "%d " , ptr->buffer[pos].id );
	printf ("\n");

	return;
}





